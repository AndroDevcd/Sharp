//
// x86-64 JIT Compiler Helper Functions
// Memory operands, VM register access, pc handling and shared checks
//

#include "x64_compiler.h"
#include "../../multitasking/fiber/fiber.h"
#include "../../multitasking/thread/sharp_thread.h"
#include "../../memory/vm_stack.h"
#include "../../memory/sharp_object.h"
#include "../jit_wrappers.h"
#include "../../virtual_machine.h"
#include <cstddef>
#include <cstring>

using namespace asmjit;

// The generated code multiplies stack indexes by shifting, keep that honest.
static_assert(sizeof(stack_item) == 16, "x64 jit assumes sizeof(stack_item) == 16");
static_assert(sizeof(object) == 8, "x64 jit assumes sizeof(object) == 8");
static_assert(sizeof(long double) == 16 || sizeof(long double) == 8, "unsupported long double size");

// ---------------------------------------------------------------------------------
// Memory operand helpers
// ---------------------------------------------------------------------------------

/**
 * registers[vmReg]
 *
 * VM registers are `long double`. With gcc/clang on x86-64 that is the 80-bit x87
 * format stored in 16 bytes, with MSVC it is a plain 64-bit double. Using x87
 * load/store with the matching operand size is exact in both cases.
 */
x86::Mem X64Compiler::vmRegister(int vmReg) {
    int32_t offset = (int32_t)(vmReg * sizeof(long double));
    if(sizeof(long double) == 16)
        return x86::tword_ptr(registersPtr, offset);
    return x86::qword_ptr(registersPtr, offset);
}

/**
 * (base + relAddress)->var      [double]
 */
x86::Mem X64Compiler::stackVar(x86::Gp base, int relAddress) {
    return x86::qword_ptr(base, (int32_t)(relAddress * (int)sizeof(stack_item) + (int)offsetof(stack_item, var)));
}

/**
 * (base + relAddress)->obj.o    [sharp_object*]
 */
x86::Mem X64Compiler::stackObj(x86::Gp base, int relAddress) {
    return x86::qword_ptr(base, (int32_t)(relAddress * (int)sizeof(stack_item)
                                          + (int)offsetof(stack_item, obj) + (int)offsetof(object, o)));
}

/**
 * Scratch slot in our own native stack frame
 */
x86::Mem X64Compiler::scratch(uint32_t size) {
    return x86::Mem(x86::rsp, SCRATCH_SLOT, size);
}

// ---------------------------------------------------------------------------------
// VM register helpers
// ---------------------------------------------------------------------------------

/**
 * Push registers[vmReg] onto the x87 stack
 */
void X64Compiler::fldRegister(int vmReg) {
    assembler->fld(vmRegister(vmReg));
}

/**
 * Pop the top of the x87 stack into registers[vmReg]
 */
void X64Compiler::fstpRegister(int vmReg) {
    assembler->fstp(vmRegister(vmReg));
}

// ---------------------------------------------------------------------------------
// Task / PC helpers
// ---------------------------------------------------------------------------------

/**
 * dest = thread->task
 */
void X64Compiler::loadTask(x86::Gp dest) {
    assembler->mov(dest, x86::qword_ptr(threadPtr, offsetof(sharp_thread, task)));
}

/**
 * task->pc = task->rom + pcReg
 *
 * REGISTERS USED: tmp0, tmp1
 */
void X64Compiler::storePC() {
    loadTask(tmp0);
    assembler->mov(tmp1, x86::qword_ptr(tmp0, offsetof(fiber, rom)));
    assembler->lea(tmp1, x86::ptr(tmp1, pcReg, 2));            // rom + pcReg * sizeof(opcode_instr)
    assembler->mov(x86::qword_ptr(tmp0, offsetof(fiber, pc)), tmp1);
}

/**
 * pcReg = task->pc - task->rom
 *
 * REGISTERS USED: tmp0
 */
void X64Compiler::loadPC() {
    loadTask(tmp0);
    assembler->mov(pcReg, x86::qword_ptr(tmp0, offsetof(fiber, pc)));
    assembler->sub(pcReg, x86::qword_ptr(tmp0, offsetof(fiber, rom)));
    assembler->sar(pcReg, 2);                                   // bytes -> instruction index
}

// ---------------------------------------------------------------------------------
// Calls into C++
// ---------------------------------------------------------------------------------

/**
 * Call a C/C++ function. Arguments must already be in arg0..arg2.
 *
 * Everything except rbx, rbp, r12-r15 is destroyed by the call: tmp0/tmp1/tmp2,
 * the argument registers and the xmm registers hold garbage afterwards. The
 * return value (if any) is in rax.
 */
void X64Compiler::callFunction(void* functionPtr) {
    assembler->mov(x86::rax, reinterpret_cast<uint64_t>(functionPtr));
    assembler->call(x86::rax);
}

// ---------------------------------------------------------------------------------
// Shared checks
// ---------------------------------------------------------------------------------

/**
 * VM macro: check_state(offset)
 *     task->pc += offset; goto state_check;
 */
void X64Compiler::emitStateCheck(size_t offset) {
    if(offset != 0) {
        assembler->add(pcReg, (int32_t)offset);
    }
    storePC();
    assembler->jmp(stateCheckLabel);
}

/**
 * Branch to an instruction whose pc is known while compiling:
 *
 *     task->pc = task->rom + targetPc;
 *     check_state(0)
 *
 * Fast path (no signal pending, which is almost always): jump straight to the
 * machine code of the target instruction. No memory is written and the jump
 * table is not used, which matters because this runs once per loop iteration.
 *
 *     if(thread->signal == 0) goto <code of instruction targetPc>;
 *
 * Slow path (a signal is pending): do exactly what emitStateCheck does.
 *
 *     pcReg = targetPc; task->pc = task->rom + pcReg; goto state_check;
 *
 * task->pc is NOT updated on the fast path. That is fine: every instruction
 * loads its own pc into pcReg when it starts, and everything that needs
 * task->pc to be right (calls, exceptions, returns, context switches) stores it
 * first.
 *
 * REGISTERS USED: tmp0, tmp1 (slow path only)
 */
void X64Compiler::emitBranchTo(size_t targetPc) {
    if(targetPc < opcodeLabels.size()) {
        assembler->cmp(x86::qword_ptr(threadPtr, offsetof(sharp_thread, signal)), 0);
        assembler->je(opcodeLabels[targetPc]);

        // compileFunction() verifies that every recorded target really is the
        // start of an instruction before the code is installed.
        branchTargets.push_back(targetPc);
    }

    // signal pending, or a target outside the function (ends up in the illegal branch handler)
    assembler->mov(pcReg.r32(), (uint32_t)targetPc);
    emitStateCheck(0);
}

/**
 * Leave the compiled function with the given jit_return_code
 */
void X64Compiler::emitReturn(int returnCode) {
    assembler->mov(x86::eax, returnCode);
    assembler->jmp(returnFromFunctionLabel);
}

/**
 * if(hasSignal(thread->signal, tsig_except)) goto catch_exception;
 *
 * hasSignal(sig, t) is ((sig >> t) & 1): the enum VALUE is the bit index.
 * Does not modify any register.
 */
void X64Compiler::emitFastExceptionCheck() {
    assembler->test(x86::qword_ptr(threadPtr, offsetof(sharp_thread, signal)), 1 << tsig_except);
    assembler->jnz(catchExceptionLabel);
}

/**
 * VM macros: grow_stack_for(n) followed by stack_overflow_check_for(n)
 *
 *     if(((task->sp-task->stack)+(n)) >= task->stackSize) task->growStack((n));
 *     if(((task->sp-task->stack)+(n)) >= task->stackLimit) throw vm_exception(vm.stack_overflow_except, "");
 *
 * REGISTERS USED: tmp0, tmp1 (and everything a C call destroys if the stack grows)
 */
void X64Compiler::emitStackCheck(int n) {
    Label noGrow = assembler->newLabel();

    loadTask(tmp0);
    assembler->mov(tmp1, x86::qword_ptr(tmp0, offsetof(fiber, sp)));
    assembler->sub(tmp1, x86::qword_ptr(tmp0, offsetof(fiber, stack)));
    assembler->sar(tmp1, 4);                                   // / sizeof(stack_item)
    assembler->add(tmp1, n);
    assembler->cmp(tmp1, x86::qword_ptr(tmp0, offsetof(fiber, stackSize)));
    assembler->jl(noGrow);

    // jit_growStack(task, n) - task->sp/fp/ptr are re-based by growStack
    storePC();
    loadTask(arg0);
    assembler->mov(arg1, n);
    callFunction(reinterpret_cast<void*>(jit_growStack));
    emitFastExceptionCheck();

    // recompute the depth, the registers did not survive the call
    loadTask(tmp0);
    assembler->mov(tmp1, x86::qword_ptr(tmp0, offsetof(fiber, sp)));
    assembler->sub(tmp1, x86::qword_ptr(tmp0, offsetof(fiber, stack)));
    assembler->sar(tmp1, 4);
    assembler->add(tmp1, n);

    assembler->bind(noGrow);
    assembler->cmp(tmp1, x86::qword_ptr(tmp0, offsetof(fiber, stackLimit)));
    assembler->jge(stackOverflowLabel);
}

/**
 * VM macro: require_object_with_value
 *     if(task->ptr==NULL || task->ptr->o == NULL) -> throw nullptr exception
 *
 * On success objectReg = task->ptr->o
 *
 * REGISTERS USED: objectReg
 */
void X64Compiler::emitRequireObjectWithValue(x86::Gp objectReg) {
    loadTask(objectReg);
    assembler->mov(objectReg, x86::qword_ptr(objectReg, offsetof(fiber, ptr)));
    assembler->test(objectReg, objectReg);
    assembler->jz(nullptrExceptionLabel);
    assembler->mov(objectReg, x86::qword_ptr(objectReg, offsetof(object, o)));
    assembler->test(objectReg, objectReg);
    assembler->jz(nullptrExceptionLabel);
}

/**
 * VM macro: require_numeric_object_with_value
 *     if(task->ptr==NULL || task->ptr->o == NULL || task->ptr->o->type > type_var) -> throw nullptr exception
 *
 * On success objectReg = task->ptr->o
 *
 * REGISTERS USED: objectReg, tmp0 (objectReg must not be tmp0)
 */
void X64Compiler::emitRequireNumericObjectWithValue(x86::Gp objectReg) {
    emitRequireObjectWithValue(objectReg);

    if(objectTypeOffset >= 0) {
        // read the bit-field directly
        assembler->movzx(x86::eax, x86::byte_ptr(objectReg, objectTypeOffset));
        assembler->and_(x86::eax, objectTypeMask);
        assembler->cmp(x86::eax, (int)type_var << objectTypeShift);
        assembler->jg(nullptrExceptionLabel);
    } else {
        // unknown layout: ask C++ (destroys objectReg, so reload it afterwards)
        assembler->mov(arg0, objectReg);
        callFunction(reinterpret_cast<void*>(jit_getObjectType));
        assembler->cmp(x86::eax, (int)type_var);
        assembler->jg(nullptrExceptionLabel);

        loadTask(objectReg);
        assembler->mov(objectReg, x86::qword_ptr(objectReg, offsetof(fiber, ptr)));
        assembler->mov(objectReg, x86::qword_ptr(objectReg, offsetof(object, o)));
    }
}

// ---------------------------------------------------------------------------------
// Jump table helpers
// ---------------------------------------------------------------------------------

void X64Compiler::initializeJumpTable(size_t opcodeCount) {
    opcodeLabels.clear();
    branchTargets.clear();
    opcodeLabels.resize(opcodeCount);

    // Create labels for each opcode position
    for (size_t i = 0; i < opcodeCount; ++i) {
        opcodeLabels[i] = assembler->newLabel();
    }
}

// ---------------------------------------------------------------------------------
// Number helpers
// ---------------------------------------------------------------------------------

/**
 * Push (long double)value on the x87 stack.
 *
 * REGISTERS USED: scratch slot
 */
void X64Compiler::emitLoadInt(int value) {
    assembler->mov(scratch(4), value);
    assembler->fild(scratch(4));
}

/**
 * xmm = value. The double is converted at compile time and its bit pattern is
 * moved through a general purpose register.
 *
 * REGISTERS USED: tmp2
 */
void X64Compiler::emitLoadDoubleConstant(x86::Vec xmm, double value) {
    uint64_t bits;
    memcpy(&bits, &value, sizeof(bits));
    assembler->mov(tmp2, bits);
    assembler->movq(xmm, tmp2);
}

/**
 * dest = (Int)registers[vmReg]
 *
 * fisttp always truncates toward zero, which is what a C++ cast from long double
 * to an integer does (plain fistp would use the current rounding mode instead).
 *
 * REGISTERS USED: dest, scratch slot
 */
void X64Compiler::emitRegisterToInt(int vmReg, x86::Gp dest) {
    fldRegister(vmReg);
    assembler->fisttp(scratch(8));
    assembler->mov(dest, scratch(8));
}

/**
 * al = 1 if the fucomip/fcomip just executed found its operands equal.
 *
 * An x87 compare reports "unordered" (one side is NaN) as ZF=1, PF=1, so equal
 * means ZF=1 and PF=0. In C++ NaN == x is false.
 *
 * REGISTERS USED: tmp0 (al), tmp1 (r10b)
 */
void X64Compiler::emitSetIfEqual() {
    assembler->sete(x86::al);
    assembler->setnp(tmp1.r8());
    assembler->and_(x86::al, tmp1.r8());
}

/**
 * registers[vmReg] = al ? 1 : 0
 *
 * REGISTERS USED: tmp0, scratch slot
 */
void X64Compiler::emitStoreBool(int vmReg) {
    assembler->movzx(x86::eax, x86::al);
    assembler->mov(scratch(4), x86::eax);
    assembler->fild(scratch(4));
    fstpRegister(vmReg);
}

/**
 * registers[destReg] = registers[leftReg] <kind> registers[rightReg]
 *
 * fcomip compares st0 with st1 and sets the flags like an UNSIGNED integer
 * compare (above/below), with "unordered" (NaN) looking like "below + equal".
 * To make every comparison false for NaN, like C++, the operands are loaded in
 * whichever order lets us test "above" / "above or equal":
 *
 *     left <  right   ==  right >  left   -> st0 = right, st1 = left, seta
 *     left <= right   ==  right >= left   -> st0 = right, st1 = left, setae
 *     left >  right                       -> st0 = left,  st1 = right, seta
 *     left >= right                       -> st0 = left,  st1 = right, setae
 *
 * REGISTERS USED: tmp0, tmp1, scratch slot
 */
void X64Compiler::emitCompare(int leftReg, int rightReg, int destReg, jit_compare kind) {
    bool rightOnTop = (kind == JIT_CMP_LT || kind == JIT_CMP_LTE || kind == JIT_CMP_EQ || kind == JIT_CMP_NE);
    if(rightOnTop) {
        fldRegister(leftReg);
        fldRegister(rightReg);     // st0 = right, st1 = left
    } else {
        fldRegister(rightReg);
        fldRegister(leftReg);      // st0 = left, st1 = right
    }

    switch(kind) {
        case JIT_CMP_LT:
        case JIT_CMP_GT:
            assembler->fcomip(x86::st1);
            assembler->seta(x86::al);
            break;
        case JIT_CMP_LTE:
        case JIT_CMP_GTE:
            assembler->fcomip(x86::st1);
            assembler->setae(x86::al);
            break;
        case JIT_CMP_EQ:
            assembler->fucomip(x86::st1);
            emitSetIfEqual();
            break;
        case JIT_CMP_NE:
            // not equal, or unordered (NaN != x is true)
            assembler->fucomip(x86::st1);
            assembler->setne(x86::al);
            assembler->setp(tmp1.r8());
            assembler->or_(x86::al, tmp1.r8());
            break;
    }

    assembler->fstp(x86::st0);     // pop the value fcomip left behind (flags/al are not affected)
    emitStoreBool(destReg);
}

/**
 * Memory operand for base[index] where base is a long double[] (sharp_object::HEAD).
 * With 16 byte long doubles the index register is multiplied by 16 in place,
 * so it holds a byte offset afterwards.
 */
x86::Mem X64Compiler::headElement(x86::Gp base, x86::Gp index) {
    if(sizeof(long double) == 16) {
        assembler->shl(index, 4);
        return x86::tword_ptr(base, index);
    }
    return x86::qword_ptr(base, index, 3);
}

// ---------------------------------------------------------------------------------
// VM stack helpers
// ---------------------------------------------------------------------------------

/**
 * slot = ++task->sp
 *
 * REGISTERS USED: tmp0, slot
 */
void X64Compiler::emitPushSlot(x86::Gp slot) {
    loadTask(tmp0);
    assembler->mov(slot, x86::qword_ptr(tmp0, offsetof(fiber, sp)));
    assembler->add(slot, (int)sizeof(stack_item));
    assembler->mov(x86::qword_ptr(tmp0, offsetof(fiber, sp)), slot);
}

/**
 * slot = task->sp--      (slot points at the item that was popped)
 *
 * REGISTERS USED: tmp0, slot
 */
void X64Compiler::emitPopSlot(x86::Gp slot) {
    loadTask(tmp0);
    assembler->mov(slot, x86::qword_ptr(tmp0, offsetof(fiber, sp)));
    assembler->sub(x86::qword_ptr(tmp0, offsetof(fiber, sp)), (int)sizeof(stack_item));
}

// ---------------------------------------------------------------------------------
// Calls into other Sharp functions
// ---------------------------------------------------------------------------------

/**
 * Emitted after every opcode that can run another Sharp function (CALL, CALLD,
 * INVOKE_DELEGATE, INT). The C++ side has already updated task->pc.
 *
 *     if(task->current != fun->originalFunction)
 *         return JIT_CONTEXT_SWITCH;   // the callee did not finish (it was
 *                                      // interrupted): unwind, the interpreter
 *                                      // carries on from task->current/task->pc
 *     pcReg = task->pc - task->rom;
 *     goto state_check;                // catches a pending exception, then
 *                                      // dispatches to pcReg
 *
 * REGISTERS USED: tmp0, tmp1
 */
void X64Compiler::emitAfterCall() {
    Label calleeFinished = assembler->newLabel();
    loadTask(tmp0);
    assembler->mov(tmp1, x86::qword_ptr(tmp0, offsetof(fiber, current)));
    assembler->cmp(tmp1, x86::qword_ptr(jitFunctionPtr, offsetof(jit_compiled_function, originalFunction)));
    assembler->je(calleeFinished);
    emitReturn(JIT_CONTEXT_SWITCH);
    assembler->bind(calleeFinished);

    loadPC();
    assembler->jmp(stateCheckLabel);
}
