//
// ARM64 JIT Compiler Implementation
//

#include "arm64_compiler.h"
#include "../../../core/opcode/opcode.h"
#include "../../../core/opcode/opcode_macros.h"
#include "../../../core/vm_register.h"
#include "../../virtual_machine.h"
#include "../../multitasking/thread/sharp_thread.h"
#include "../../memory/vm_stack.h"
#include "../jit_wrappers.h"
#include <cstddef>  // For offsetof

using namespace asmjit;

Arm64Compiler::Arm64Compiler() {
    // Explicitly set ARM64 architecture for cross-compilation
    code = new CodeHolder();
    Environment arm64Env(Arch::kAArch64);
    code->init(arm64Env);
    assembler = new a64::Assembler(code);
    currentFunction = nullptr;
    
    // Initialize labeled ARM64 registers
    threadPtr = a64::x19;     // Callee-saved register for thread context pointer
    registersPtr = a64::x20;  // Callee-saved register for registers array pointer
    jitFunctionPtr = a64::x25; // Callee-saved register for jit_compiled_function pointer
    tempVec1 = a64::d8;       // Callee-saved vector register 1 for floating point
    tempVec2 = a64::d9;       // Callee-saved vector register 2 for floating point
    tempReg1 = a64::x21;      // Callee-saved temp register 1
    tempReg2 = a64::x22;      // Callee-saved temp register 2
    tempReg3 = a64::x23;      // Callee-saved temp register 3
    tempReg4 = a64::x26;      // Callee-saved temp register 4
    tempReg5 = a64::x28;      // Callee-saved temp register 5
    tempReg6 = a64::x12;      // Caller-saved temp register 6
    tempReg7 = a64::x13;      // Caller-saved temp register 7  
    tempReg8 = a64::x14;      // Caller-saved temp register 8
    tempReg9 = a64::x15;      // Caller-saved temp register 9
    tempReg32_1 = a64::w10;   // Caller-saved 32-bit temp register 1
    tempReg32_2 = a64::w11;   // Caller-saved 32-bit temp register 2
    jumpTablePtr = a64::x24;  // Callee-saved register for jump table pointer
    pcReg = a64::x27;         // Callee-saved register for current PC tracking
    
    // Initialize standard ARM64 registers for consistency
    returnReg = a64::x0;      // Function return value register (64-bit)
    framePtr = a64::x29;      // Frame pointer
    linkReg = a64::x30;       // Link register
    stackPtr = a64::sp;       // Stack pointer
    
    // Initialize labels
    stateCheckLabel = assembler->newLabel();
    catchExceptionLabel = assembler->newLabel();
    returnFromFunctionLabel = assembler->newLabel();
    illegalBranchLabel = assembler->newLabel();
    growStackLabel = assembler->newLabel();
    stackOverflowLabel = assembler->newLabel();
    objectValueCheckLabel = assembler->newLabel();
    numericObjectCheckLabel = assembler->newLabel();

    // Initialize PC tracking
    currentPC = 0;
}

Arm64Compiler::~Arm64Compiler() {
    if(assembler) {
        delete assembler;
        assembler = nullptr;
    }
    if(code) {
        delete code;
        code = nullptr;
    }
}

bool Arm64Compiler::compileFunction(sharp_function* function, jit_compiled_function* output) {
    if(!should_compile_function(function)) {
        return false;
    }

    resetCodeHolder();

    count++;
    currentFunction = function;
    // Initialize jump table for this function
    initializeJumpTable(function->bytecodeSize);
    
    if(!setupFunctionPrologue()) {
        return false;
    }
    
    // Translate Sharp IR opcodes to ARM64 assembly
    uint32_t* pc = function->bytecode;
    uint32_t* endPc = pc + function->bytecodeSize;
    currentPC = 0;
    
    while(pc < endPc) {
        currentPC = pc - function->bytecode;
        // Bind the label for this PC position
        setJumpTableEntry(currentPC, opcodeLabels[currentPC]);

        // Section separator for debugging
        assembler->nop();
        assembler->nop();
        assembler->nop();
        assembler->nop();
        // Set pcReg to current PC for this instruction
        assembler->mov(pcReg, currentPC);

//        // Call debug wrapper function
//        assembler->mov(a64::x0, *pc);  // First parameter: opcode instruction
//        assembler->mov(a64::x1, currentPC);  // Second parameter: current PC
//        callStaticFunction(reinterpret_cast<void*>(jit_instructionStart));

        if(!translateOpcode(*pc, &pc, function)) {
            return false;
        }
        pc++;
    }

    // Safety measure in case the ret instruction is skipped or fails for some reason
    // (should never get to this point)
    emitReturn(JIT_OK);

    if(!setupFunctionEpilogue()) {
        return false;
    }
    
    // Finalize and get the compiled code
    void* func;
    Error err = runtime.add(&func, code);
    if(err) {
        return false;
    }
    
    // Allocate and populate the jump table
    output->jumpTableSize = function->bytecodeSize;
    output->jumpTable = new void*[output->jumpTableSize];
    
    // Get the actual addresses of each opcode label
    size_t illegalBranchOffset = code->labelOffset(illegalBranchLabel);
    void* illegalBranchAddress = static_cast<char*>(func) + illegalBranchOffset;
    
    for (size_t i = 0; i < output->jumpTableSize; ++i) {
        if (code->isLabelBound(opcodeLabels[i])) {
            size_t offset = code->labelOffset(opcodeLabels[i]);
            output->jumpTable[i] = static_cast<char*>(func) + offset;
        } else {
            // Label not bound (multi-slot opcode), point to illegal branch handler
            output->jumpTable[i] = illegalBranchAddress;
        }
    }
    
    output->compiledCode = reinterpret_cast<jit_function_ptr>(func);
    output->codeSize = code->codeSize();
    output->isCompiled = true;
    output->originalFunction = function;

    return true;
}

void Arm64Compiler::releaseCompiledFunction(jit_compiled_function* compiledFunc) {
    if(compiledFunc && compiledFunc->compiledCode) {
        runtime.release(compiledFunc->compiledCode);
        compiledFunc->compiledCode = nullptr;
        compiledFunc->isCompiled = false;
    }
}

bool Arm64Compiler::translateOpcode(uint32_t opcode, uint32_t** pc, sharp_function* function) {
    uint8_t op = GET_OP(opcode);
    
    switch(op) {
        // Basic opcodes
        case Opcode::NOP:
            return emit_nop();
        case Opcode::INT:
            return emit_int(GET_Da(opcode));
        case Opcode::MOVI: {
            bool result = emit_movi(*(*pc + 1), GET_Da(opcode));
            (*pc)++; // Two-word instruction - increment PC for consumed word
            return result;
        }
        case Opcode::RET:
            return emit_ret(GET_Da(opcode));
        case Opcode::HLT:
            return emit_hlt();
            
        // Array and object operations
        case Opcode::NEWARRAY:
            return emit_newarray(GET_Ca(opcode), GET_Cb(opcode));
        case Opcode::CAST:
            return emit_cast(GET_Da(opcode));
        case Opcode::NEWCLASS: {
            bool result = emit_newclass(*(*pc + 1));
            (*pc)++; // Two-word instruction - increment PC for consumed word
            return result;
        }
        case Opcode::NEWSTRING:
            return emit_newstring(GET_Da(opcode));
        case Opcode::NEWOBJARRAY:
            return emit_newobjarray(GET_Da(opcode));
        case Opcode::NEWCLASSARRAY: {
            bool result = emit_newclassarray(GET_Ca(opcode), *(*pc + 1));
            (*pc)++; // Two-word instruction - increment PC for consumed word
            return result;
        }
        case Opcode::DEL:
            return emit_del();
        case Opcode::PUSHOBJ:
            return emit_pushobj();
        case Opcode::POPOBJ:
            return emit_popobj();
        case Opcode::POPOBJ_2:
            return emit_popobj_2();
            
        // Move operations
        case Opcode::MOV8:
            return emit_mov8(GET_Ca(opcode), GET_Cb(opcode));
        case Opcode::MOV16:
            return emit_mov16(GET_Ca(opcode), GET_Cb(opcode));
        case Opcode::MOV32:
            return emit_mov32(GET_Ca(opcode), GET_Cb(opcode));
        case Opcode::MOV64:
            return emit_mov64(GET_Ca(opcode), GET_Cb(opcode));
        case Opcode::MOVU8:
            return emit_movu8(GET_Ca(opcode), GET_Cb(opcode));
        case Opcode::MOVU16:
            return emit_movu16(GET_Ca(opcode), GET_Cb(opcode));
        case Opcode::MOVU32:
            return emit_movu32(GET_Ca(opcode), GET_Cb(opcode));
        case Opcode::MOVU64:
            return emit_movu64(GET_Ca(opcode), GET_Cb(opcode));
        case Opcode::MOVR:
            return emit_movr(GET_Ca(opcode), GET_Cb(opcode));
        case Opcode::MOVL:
            return emit_movl(GET_Da(opcode));
        case Opcode::MOVSL:
            return emit_movsl(GET_Da(opcode));
        case Opcode::MOVN: {
            bool result = emit_movn(*(*pc + 1));
            (*pc)++; // Two-word instruction - increment PC for consumed word
            return result;
        }
        case Opcode::MOVG:
            return emit_movg(GET_Da(opcode));
        case Opcode::MOVND:
            return emit_movnd(GET_Da(opcode));
        case Opcode::MOV_ABS:
            return emit_movabs(GET_Da(opcode));
        case Opcode::LOAD_ABS:
            return emit_loadabs(GET_Da(opcode));
        case Opcode::RMOV:
            return emit_rmov(GET_Ca(opcode), GET_Cb(opcode));
        case Opcode::SMOV:
            return emit_smov(GET_Ca(opcode), GET_Cb(opcode));
        case Opcode::SMOVR:
            return emit_smovr(GET_Ca(opcode), GET_Cb(opcode));
        case Opcode::SMOVR_2:
            return emit_smovr_2(GET_Ca(opcode), GET_Cb(opcode));
        case Opcode::SMOVR_3:
            return emit_smovr_3(GET_Da(opcode));
        case Opcode::SMOVR_4: {
            bool result = emit_smovr_4(GET_Ca(*(*pc + 1)), GET_Cb(*(*pc + 1)));
            (*pc)++; // Two-word instruction - increment PC for consumed word
            return result;
        }
        case Opcode::IMOV:
            return emit_imov(GET_Da(opcode));
            
        // Store and load operations
        case Opcode::RSTORE:
            return emit_rstore(GET_Da(opcode));
        case Opcode::ISTORE: {
            bool result = emit_istore(*(*pc + 1));
            (*pc)++; // Two-word instruction - increment PC for consumed word
            return result;
        }
        case Opcode::ISTOREL: {
            bool result = emit_istorel(GET_Ca(*(*pc + 1)), GET_Cb(*(*pc + 1)));
            (*pc)++; // Two-word instruction - increment PC for consumed word
            return result;
        }
        case Opcode::LOADL:
            return emit_loadl(GET_Ca(opcode), GET_Cb(opcode));
        case Opcode::LOADVAL:
            return emit_loadval(GET_Da(opcode));
        case Opcode::LOADPC:
            return emit_loadpc(GET_Da(opcode));
        case Opcode::IALOAD:
            return emit_iaload(GET_Ca(opcode), GET_Cb(opcode));
        case Opcode::ILOAD:
            return emit_iload(GET_Da(opcode));
        case Opcode::LDC:
            return emit_ldc(GET_Ca(opcode), GET_Cb(opcode));
            
        // Arithmetic operations
        case Opcode::ADD:
            return emit_add(GET_Ba(opcode), GET_Bb(opcode), GET_Bc(opcode));
        case Opcode::SUB:
            return emit_sub(GET_Ba(opcode), GET_Bb(opcode), GET_Bc(opcode));
        case Opcode::MUL:
            return emit_mul(GET_Ba(opcode), GET_Bb(opcode), GET_Bc(opcode));
        case Opcode::DIV:
            return emit_div(GET_Ba(opcode), GET_Bb(opcode), GET_Bc(opcode));
        case Opcode::MOD:
            return emit_mod(GET_Ba(opcode), GET_Bb(opcode), GET_Bc(opcode));
        case Opcode::IADD: {
            bool result = emit_iadd(GET_Ca(opcode), *(*pc + 1));
            (*pc)++; // Two-word instruction - increment PC for consumed word
            return result;
        }
        case Opcode::ISUB: {
            bool result = emit_isub(GET_Ca(opcode), *(*pc + 1));
            (*pc)++; // Two-word instruction - increment PC for consumed word
            return result;
        }
        case Opcode::IMUL: {
            bool result = emit_imul(GET_Ca(opcode), *(*pc + 1));
            (*pc)++; // Two-word instruction - increment PC for consumed word
            return result;
        }
        case Opcode::IDIV: {
            bool result = emit_idiv(GET_Ca(opcode), *(*pc + 1));
            (*pc)++; // Two-word instruction - increment PC for consumed word
            return result;
        }
        case Opcode::IMOD: {
            bool result = emit_imod(GET_Ca(opcode), *(*pc + 1));
            (*pc)++; // Two-word instruction - increment PC for consumed word
            return result;
        }
        case Opcode::ADDL:
            return emit_addl(GET_Ca(opcode), GET_Cb(opcode));
        case Opcode::SUBL:
            return emit_subl(GET_Ca(opcode), GET_Cb(opcode));
        case Opcode::MULL:
            return emit_mull(GET_Ca(opcode), GET_Cb(opcode));
        case Opcode::DIVL:
            return emit_divl(GET_Ca(opcode), GET_Cb(opcode));
        case Opcode::MODL:
            return emit_modl(GET_Ca(opcode), GET_Cb(opcode));
        case Opcode::IADDL: {
            bool result = emit_iaddl(*(*pc + 1), GET_Da(opcode));
            (*pc)++; // Two-word instruction - increment PC for consumed word
            return result;
        }
        case Opcode::ISUBL: {
            bool result = emit_isubl(*(*pc + 1), GET_Da(opcode));
            (*pc)++; // Two-word instruction - increment PC for consumed word
            return result;
        }
        case Opcode::IMULL: {
            bool result = emit_imull(*(*pc + 1), GET_Da(opcode));
            (*pc)++; // Two-word instruction - increment PC for consumed word
            return result;
        }
        case Opcode::IDIVL: {
            bool result = emit_idivl(*(*pc + 1), GET_Da(opcode));
            (*pc)++; // Two-word instruction - increment PC for consumed word
            return result;
        }
        case Opcode::IMODL: {
            bool result = emit_imodl(*(*pc + 1), GET_Da(opcode));
            (*pc)++; // Two-word instruction - increment PC for consumed word
            return result;
        }
        case Opcode::INC:
            return emit_inc(GET_Da(opcode));
        case Opcode::DEC:
            return emit_dec(GET_Da(opcode));
        case Opcode::NEG:
            return emit_neg(GET_Ca(opcode), GET_Cb(opcode));
        case Opcode::EXP:
            return emit_exp(GET_Ba(opcode), GET_Bb(opcode), GET_Bc(opcode));
        case Opcode::ISADD: {
            bool result = emit_isadd(GET_Ca(*(*pc + 1)), GET_Cb(*(*pc + 1)));
            (*pc)++; // Two-word instruction - increment PC for consumed word
            return result;
        }
            
        // Bitwise operations
        case Opcode::AND:
            return emit_and(GET_Ca(opcode), GET_Cb(opcode));
        case Opcode::UAND:
            return emit_uand(GET_Ca(opcode), GET_Cb(opcode));
        case Opcode::OR:
            return emit_or(GET_Ca(opcode), GET_Cb(opcode));
        case Opcode::XOR:
            return emit_xor(GET_Ca(opcode), GET_Cb(opcode));
        case Opcode::ANDL:
            return emit_andl(GET_Ca(opcode), GET_Cb(opcode));
        case Opcode::ORL:
            return emit_orl(GET_Ca(opcode), GET_Cb(opcode));
        case Opcode::XORL:
            return emit_xorl(GET_Ca(opcode), GET_Cb(opcode));
        case Opcode::SHL:
            return emit_shl(GET_Ba(opcode), GET_Bb(opcode), GET_Bc(opcode));
        case Opcode::SHR:
            return emit_shr(GET_Ba(opcode), GET_Bb(opcode), GET_Bc(opcode));
        case Opcode::NOT:
            return emit_not(GET_Ca(opcode), GET_Cb(opcode));
            
        // Stack operations
        case Opcode::POP:
            return emit_pop();
        case Opcode::POPL:
            return emit_popl(GET_Da(opcode));
        case Opcode::IPOPL:
            return emit_ipopl(GET_Da(opcode));
        case Opcode::PUSHNULL:
            return emit_pushnull();
        case Opcode::IPUSHL:
            return emit_ipushl(GET_Da(opcode));
        case Opcode::PUSHL:
            return emit_pushl(GET_Da(opcode));
        case Opcode::DUP:
            return emit_dup();
        case Opcode::SWAP:
            return emit_swap();
            
        // Comparison operations
        case Opcode::LT:
            return emit_lt(GET_Ca(opcode), GET_Cb(opcode));
        case Opcode::GT:
            return emit_gt(GET_Ca(opcode), GET_Cb(opcode));
        case Opcode::LTE:
            return emit_lte(GET_Ca(opcode), GET_Cb(opcode));
        case Opcode::GTE:
            return emit_gte(GET_Ca(opcode), GET_Cb(opcode));
        case Opcode::TEST:
            return emit_test(GET_Ca(opcode), GET_Cb(opcode));
        case Opcode::TNE:
            return emit_tne(GET_Ca(opcode), GET_Cb(opcode));
        case Opcode::ITEST:
            return emit_itest(GET_Da(opcode));
        case Opcode::CMP: {
            bool result = emit_cmp(GET_Ca(opcode), *(*pc + 1));
            (*pc)++; // Two-word instruction - increment PC for consumed word
            return result;
        }
        case Opcode::IS: {
            bool result = emit_is(GET_Ca(opcode), *(*pc + 1));
            (*pc)++; // Two-word instruction - increment PC for consumed word
            return result;
        }
            
        // Control flow operations
        case Opcode::JMP:
            return emit_jmp(GET_Da(opcode));
        case Opcode::JE:
            return emit_je(GET_Da(opcode));
        case Opcode::JNE:
            return emit_jne(GET_Da(opcode));
        case Opcode::IFE:
            return emit_ife();
        case Opcode::IFNE:
            return emit_ifne();
        case Opcode::BRH:
            return emit_brh();
        case Opcode::SKIP:
            return emit_skip(GET_Da(opcode));
        case Opcode::SKPE:
            return emit_skpe(GET_Ca(opcode), GET_Cb(opcode));
        case Opcode::SKNE:
            return emit_skne(GET_Ca(opcode), GET_Cb(opcode));
            
        // Function operations
        case Opcode::CALL:
            return emit_call(GET_Da(opcode));
        case Opcode::CALLD:
            return emit_calld(GET_Da(opcode));
        case Opcode::INVOKE_DELEGATE: {
            bool result = emit_invoke_delegate(GET_Ca(*(*pc + 1)), GET_Cb(*(*pc + 1)), GET_Ca(opcode));
            (*pc)++; // Two-word instruction - increment PC for consumed word
            return result;
        }
        case Opcode::RETURNVAL:
            return emit_returnval(GET_Da(opcode));
        case Opcode::RETURNOBJ:
            return emit_returnobj();
            
        // Utility operations
        case Opcode::SIZEOF:
            return emit_sizeof(GET_Da(opcode));
        case Opcode::PUT:
            return emit_put(GET_Da(opcode));
        case Opcode::PUTC:
            return emit_putc(GET_Da(opcode));
        case Opcode::CHECKLEN:
            return emit_checklen(GET_Da(opcode));
        case Opcode::CHECKNULL:
            return emit_checknull(GET_Da(opcode));
        case Opcode::GET:
            return emit_get(GET_Da(opcode));
        case Opcode::SLEEP:
            return emit_sleep(GET_Da(opcode));
        case Opcode::LOCK:
            return emit_lock();
        case Opcode::ULOCK:
            return emit_ulock();
        case Opcode::THROW:
            return emit_throw();
        case Opcode::VARCAST:
            return emit_varcast(GET_Ca(opcode), GET_Cb(opcode));
        case Opcode::TLS_MOVL:
            return emit_tls_movl(GET_Da(opcode));
            
        default:
            // Unsupported opcode - return false to fall back to interpreter
            return false;
    }
}

bool Arm64Compiler::setupFunctionPrologue() {
    // ARM64 calling convention: x0-x7 are argument registers
    // JIT function signature: int jit_func(sharp_thread* thread, jit_compiled_function* fun, long double* registers)
    // x0 = sharp_thread* thread
    // x1 = jit_compiled_function* fun
    // x2 = long double* registers
    
    // Save frame pointer and link register
    assembler->stp(framePtr, linkReg, a64::ptr(stackPtr, -16).pre());
    
    // Set up frame pointer
    assembler->mov(framePtr, stackPtr);
    
    // Save all callee-saved registers we use (ARM64 requires this)
    // Save general purpose callee-saved registers: x19, x20, x21, x22, x23, x24, x25, x26, x27, x28
    assembler->stp(threadPtr, registersPtr, a64::ptr(stackPtr, -16).pre());
    assembler->stp(tempReg1, tempReg2, a64::ptr(stackPtr, -16).pre());
    assembler->stp(tempReg3, jumpTablePtr, a64::ptr(stackPtr, -16).pre());
    assembler->stp(jitFunctionPtr, tempReg4, a64::ptr(stackPtr, -16).pre());  // Save x25, x26 together
    assembler->stp(pcReg, tempReg5, a64::ptr(stackPtr, -16).pre());  // Save x27, x28 together
    
    // Save additional caller-saved registers we use: x12, x13, x14, x15, w10, w11 (tempReg6, tempReg7, tempReg8, tempReg9, tempReg32_1, tempReg32_2)
    assembler->stp(tempReg6, tempReg7, a64::ptr(stackPtr, -16).pre());
    assembler->stp(tempReg8, tempReg9, a64::ptr(stackPtr, -16).pre());  // Save x14, x15 together, maintain 16-byte alignment
    assembler->stp(a64::x10, a64::x11, a64::ptr(stackPtr, -16).pre());  // Save x10, x11 together (full 64-bit registers for 32-bit temps)
    
    // Save vector callee-saved registers: d8, d9
    assembler->stp(tempVec1, tempVec2, a64::ptr(stackPtr, -16).pre());
    
    // Reserve minimal stack space (16 bytes for alignment)
    assembler->sub(stackPtr, stackPtr, 16);
    
    // Store parameters in callee-saved registers
    assembler->mov(threadPtr, a64::x0);      // threadPtr = sharp_thread* (first parameter)
    assembler->mov(jitFunctionPtr, a64::x1); // jitFunctionPtr = jit_compiled_function* (second parameter)
    assembler->mov(registersPtr, a64::x2);   // registersPtr = long double* registers (third parameter)
    
    // Load jump table address from jit_compiled_function->jumpTable
    // jitFunctionPtr now contains the jit_compiled_function pointer
    assembler->ldr(jumpTablePtr, a64::ptr(jitFunctionPtr, offsetof(jit_compiled_function, jumpTable)));

    // load the current pc value should be 0 if first time function call
    loadPC();

    /*
     * Context Switch Frame Rebuild Logic
     * ==================================
     * 
     * When a JIT function is called, it might be resuming from a context switch.
     * In this case, we need to rebuild the call stack to restore proper execution context.
     * 
     * This implements the C++ logic:
     *   if(thread->task->current != fun->originalFunction) {
     *       sharp_function *frame = get_next_frame(fun->originalFunction);
     *       invoke_next_frame(frame, false);
     *       goto check_state(pgReg);  // Check for additional thread signals
     *   }
     * 
     * The comparison checks if the current executing function differs from what
     * this JIT function expects. If different, it means we're resuming after a
     * context switch and need to rebuild the call stack hierarchy before continuing.
     */
    
    // Frame rebuild code: if(thread->task->current != fun->originalFunction)
    Label skipFrameRebuild = assembler->newLabel();
    
    // Load thread->task->current
    assembler->ldr(tempReg1, a64::ptr(threadPtr, offsetof(sharp_thread, task))); // tempReg1 = thread->task
    assembler->ldr(tempReg2, a64::ptr(tempReg1, offsetof(fiber, current)));      // tempReg2 = thread->task->current
    
    // Load fun->originalFunction (jitFunctionPtr points to jit_compiled_function)
    assembler->ldr(tempReg3, a64::ptr(jitFunctionPtr, offsetof(jit_compiled_function, originalFunction))); // tempReg3 = fun->originalFunction
    
    // Compare thread->task->current with fun->originalFunction
    assembler->cmp(tempReg2, tempReg3);
    assembler->b_eq(skipFrameRebuild);  // Skip if they are equal
    
    // Call get_next_frame(fun->originalFunction)
    assembler->mov(a64::x0, tempReg3);  // First parameter: fun->originalFunction
    callStaticFunction(reinterpret_cast<void*>(get_next_frame));
    assembler->mov(tempReg1, returnReg); // Save returned sharp_function* frame
    
    // Call invoke_next_frame(frame, false)
    assembler->mov(a64::x0, tempReg1);  // First parameter: frame
    assembler->mov(a64::x1, 0);         // Second parameter: false
    callStaticFunction(reinterpret_cast<void*>(invoke_next_frame));
    loadPC();

    // Jump to state check
    assembler->b(stateCheckLabel);

    assembler->bind(skipFrameRebuild);
    
    return true;
}

bool Arm64Compiler::setupFunctionEpilogue() {

    // Section separator for debugging
    assembler->nop();
    assembler->nop();
    assembler->nop();
    assembler->nop();
    assembler->hint(1);
    generateNumericObjectCheckSection();

    // Section separator for debugging
    assembler->nop();
    assembler->nop();
    assembler->nop();
    assembler->nop();
    assembler->hint(2);

    generateObjectValueCheckSection();

    // Section separator for debugging
    assembler->nop();
    assembler->nop();
    assembler->nop();
    assembler->nop();
    assembler->hint(3);

    // Generate stack operation sections first (most commonly used)
    generateGrowStackSection();
    
    // Section separator for debugging
    assembler->nop();
    assembler->nop();
    assembler->nop();
    assembler->nop();
    assembler->hint(4);
    
    generateStackOverflowSection();
    
    // Section separator for debugging
    assembler->nop();
    assembler->nop();
    assembler->nop();
    assembler->nop();
    assembler->hint(5);
    
    // Generate state check section
    generateStateCheckSection();
    
    // Section separator for debugging
    assembler->nop();
    assembler->nop();
    assembler->nop();
    assembler->nop();
    assembler->hint(6);
    
    // Generate exception handler section
    generateExceptionHandlerSection();
    
    // Section separator for debugging
    assembler->nop();
    assembler->nop();
    assembler->nop();
    assembler->nop();
    assembler->hint(7);
    
    // Generate centralized return section
    generateReturnSection();

    // Section separator for debugging
    assembler->nop();
    assembler->nop();
    assembler->nop();
    assembler->nop();
    assembler->hint(8);

    generateIllegalBranchSection();

    // Section separator for debugging
    assembler->nop();
    assembler->nop();
    assembler->nop();
    assembler->nop();
    
    return true;
}

void Arm64Compiler::resetCodeHolder() {
    // AsmJit recommended approach: Reset CodeHolder with soft policy (preserves memory allocation)
    // This automatically detaches all emitters and clears the code content for reuse
    code->reset(ResetPolicy::kSoft);

    // Delete the old assembler since it's now detached
    if(assembler) {
        delete assembler;
        assembler = nullptr;
    }

    // Re-initialize with ARM64 environment (required after reset)
    Environment arm64Env(Arch::kAArch64);
    code->init(arm64Env);

    // Create a fresh assembler and attach it to the reset code holder
    assembler = new a64::Assembler(code);
    
    // Recreate all labels with the new assembler for fresh start
    stateCheckLabel = assembler->newLabel();
    catchExceptionLabel = assembler->newLabel();
    returnFromFunctionLabel = assembler->newLabel();
    illegalBranchLabel = assembler->newLabel();
    growStackLabel = assembler->newLabel();
    stackOverflowLabel = assembler->newLabel();
    objectValueCheckLabel = assembler->newLabel();
    numericObjectCheckLabel = assembler->newLabel();
    currentFunction = nullptr;
}

// Helper functions and section implementations are now in separate files


