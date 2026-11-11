//
// x86-64 JIT Compiler Implementation
//

#include "x64_compiler.h"
#include "../../../core/opcode/opcode.h"
#include "../../../core/opcode/opcode_macros.h"
#include "../../../core/vm_register.h"
#include "../../virtual_machine.h"
#include "../../multitasking/thread/sharp_thread.h"
#include "../../multitasking/fiber/fiber.h"
#include "../../memory/vm_stack.h"
#include "../../memory/sharp_object.h"
#include "../jit_wrappers.h"
#include "../../telemetry/telemetry.h"
#include <cstddef>  // For offsetof
#include <cstring>

using namespace asmjit;

X64Compiler::X64Compiler() {
    code = new CodeHolder();
    Environment x64Env(Arch::kX64);
    code->init(x64Env);
    code->setErrorHandler(&errorHandler);
    assembler = new x86::Assembler(code);
    currentFunction = nullptr;

    // Callee-saved registers that hold our context for the whole function
    threadPtr = x86::rbx;
    registersPtr = x86::r12;
    jitFunctionPtr = x86::r13;
    jumpTablePtr = x86::r14;
    pcReg = x86::r15;

    // Caller-saved temporaries (never argument registers on any calling convention)
    tmp0 = x86::rax;
    tmp1 = x86::r10;
    tmp2 = x86::r11;

    // Calling convention of the C++ code we call into / are called from
#if defined(_WIN32) || defined(__CYGWIN__)
    arg0 = x86::rcx;
    arg1 = x86::rdx;
    arg2 = x86::r8;
#else
    arg0 = x86::rdi;
    arg1 = x86::rsi;
    arg2 = x86::rdx;
#endif

    createLabels();
    probeObjectTypeLayout();
    currentPC = 0;
}

X64Compiler::~X64Compiler() {
    if(assembler) {
        delete assembler;
        assembler = nullptr;
    }
    if(code) {
        delete code;
        code = nullptr;
    }
}

void X64Compiler::createLabels() {
    stateCheckLabel = assembler->newLabel();
    catchExceptionLabel = assembler->newLabel();
    returnFromFunctionLabel = assembler->newLabel();
    illegalBranchLabel = assembler->newLabel();
    nullptrExceptionLabel = assembler->newLabel();
    stackOverflowLabel = assembler->newLabel();
}

/**
 * sharp_object::type is a bit-field, so there is no offsetof() for it. Instead of
 * guessing how the C++ compiler packed it, set every bit of the field on a zeroed
 * object and look at which bits of which byte changed.
 */
void X64Compiler::probeObjectTypeLayout() {
    sharp_object probe;
    memset((void*)&probe, 0, sizeof(sharp_object));
    probe.type = 0x1f;

    objectTypeOffset = -1;
    objectTypeMask = 0;
    objectTypeShift = 0;

    int bytesUsed = 0;
    const unsigned char *raw = reinterpret_cast<const unsigned char*>(&probe);
    for(size_t i = 0; i < sizeof(sharp_object); i++) {
        if(raw[i] != 0) {
            bytesUsed++;
            objectTypeOffset = (int)i;
            objectTypeMask = raw[i];
        }
    }

    if(bytesUsed != 1) {
        objectTypeOffset = -1; // field spans bytes: fall back to calling jit_getObjectType
        return;
    }

    while(((objectTypeMask >> objectTypeShift) & 1) == 0)
        objectTypeShift++;
}

bool X64Compiler::compileFunction(sharp_function* function, jit_compiled_function* output) {
    if(!should_compile_function(function)) {
        return false;
    }

    resetCodeHolder();

    currentFunction = function;
    // Initialize jump table for this function
    initializeJumpTable(function->bytecodeSize);

    if(!setupFunctionPrologue()) {
        return false;
    }

    // Translate Sharp IR opcodes to x86-64 assembly
    uint32_t* pc = function->bytecode;
    uint32_t* endPc = pc + function->bytecodeSize;
    currentPC = 0;

    while(pc < endPc) {
        currentPC = pc - function->bytecode;
        // Bind the label for this PC position
        assembler->bind(opcodeLabels[currentPC]);

        // Set pcReg to current PC for this instruction
        assembler->mov(pcReg.r32(), (uint32_t)currentPC);

        uint32_t instruction = *pc; // translateOpcode may advance pc past a two-word instruction
        if(!translateOpcode(instruction, &pc, function)) {
            // this opcode is not implemented by the jit yet: the function stays interpreted
            TELEMETRY(telemetry_jit_blocked(function, GET_OP(instruction));)
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

    // Every direct branch must land on the first word of an instruction. A target
    // in the middle of a two-word instruction has no machine code (its label was
    // never bound), so such a function cannot be compiled.
    for(size_t target : branchTargets) {
        if(!code->isLabelBound(opcodeLabels[target])) {
            return false;
        }
    }

    // If AsmJit rejected any instruction it emitted nothing for it, so the
    // function would be silently broken. Fall back to the interpreter instead.
    if(errorHandler.failed) {
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
            // Label not bound (second word of a two-word opcode), point to illegal branch handler
            output->jumpTable[i] = illegalBranchAddress;
        }
    }

    output->compiledCode = reinterpret_cast<jit_function_ptr>(func);
    output->codeSize = code->codeSize();
    output->isCompiled = true;
    output->originalFunction = function;

#ifdef JIT_DEBUG
    fprintf(stderr, "jit: compiled %s (%d words -> %zu bytes)\n",
            function->fullName.c_str(), function->bytecodeSize, output->codeSize);
#endif

    return true;
}

void X64Compiler::releaseCompiledFunction(jit_compiled_function* compiledFunc) {
    if(compiledFunc && compiledFunc->compiledCode) {
        runtime.release(compiledFunc->compiledCode);
        compiledFunc->compiledCode = nullptr;
        compiledFunc->isCompiled = false;
    }
}

// NOTE: operand decoding must stay identical to the interpreter (opcode_macros.h)
// and to Arm64Compiler::translateOpcode.
bool X64Compiler::translateOpcode(uint32_t opcode, uint32_t** pc, sharp_function* function) {
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
            bool result = emit_newclassarray(GET_Da(opcode), (int32_t)*(*pc + 1));
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
            bool result = emit_smovr_4(GET_Da(opcode), (int32_t)*(*pc + 1));
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
            bool result = emit_istorel(GET_Da(opcode), (int32_t)*(*pc + 1));
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
            return emit_add(GET_Bc(opcode), GET_Ba(opcode), GET_Bb(opcode)) /* encoded as left, right, out */;
        case Opcode::SUB:
            return emit_sub(GET_Bc(opcode), GET_Ba(opcode), GET_Bb(opcode)) /* encoded as left, right, out */;
        case Opcode::MUL:
            return emit_mul(GET_Bc(opcode), GET_Ba(opcode), GET_Bb(opcode)) /* encoded as left, right, out */;
        case Opcode::DIV:
            return emit_div(GET_Bc(opcode), GET_Ba(opcode), GET_Bb(opcode)) /* encoded as left, right, out */;
        case Opcode::MOD:
            return emit_mod(GET_Bc(opcode), GET_Ba(opcode), GET_Bb(opcode)) /* encoded as left, right, out */;
        case Opcode::IADD: {
            bool result = emit_iadd(GET_Da(opcode), (int32_t)*(*pc + 1));
            (*pc)++; // Two-word instruction - increment PC for consumed word
            return result;
        }
        case Opcode::ISUB: {
            bool result = emit_isub(GET_Da(opcode), (int32_t)*(*pc + 1));
            (*pc)++; // Two-word instruction - increment PC for consumed word
            return result;
        }
        case Opcode::IMUL: {
            bool result = emit_imul(GET_Da(opcode), (int32_t)*(*pc + 1));
            (*pc)++; // Two-word instruction - increment PC for consumed word
            return result;
        }
        case Opcode::IDIV: {
            bool result = emit_idiv(GET_Da(opcode), (int32_t)*(*pc + 1));
            (*pc)++; // Two-word instruction - increment PC for consumed word
            return result;
        }
        case Opcode::IMOD: {
            bool result = emit_imod(GET_Da(opcode), (int32_t)*(*pc + 1));
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
            return emit_exp(GET_Bc(opcode), GET_Ba(opcode), GET_Bb(opcode)) /* encoded as left, right, out */;
        case Opcode::ISADD: {
            bool result = emit_isadd(GET_Da(opcode), (int32_t)*(*pc + 1));
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
            return emit_shl(GET_Bc(opcode), GET_Ba(opcode), GET_Bb(opcode)) /* encoded as left, right, out */;
        case Opcode::SHR:
            return emit_shr(GET_Bc(opcode), GET_Ba(opcode), GET_Bb(opcode)) /* encoded as left, right, out */;
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
            bool result = emit_cmp(GET_Da(opcode), (int32_t)*(*pc + 1));
            (*pc)++; // Two-word instruction - increment PC for consumed word
            return result;
        }
        case Opcode::IS: {
            bool result = emit_is(GET_Da(opcode), (int32_t)*(*pc + 1));
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
            bool result = emit_invoke_delegate(GET_Da(opcode), GET_Cb(*(*pc + 1)), GET_Ca(*(*pc + 1)) != 0);
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

bool X64Compiler::setupFunctionPrologue() {
    // Function signature: int (*jit_function_ptr)(sharp_thread* thread, jit_compiled_function* fun, long double* registers)
    //   arg0 = sharp_thread* thread
    //   arg1 = jit_compiled_function* fun
    //   arg2 = long double* registers

    // Standard frame + save every callee-saved register we use
    assembler->push(x86::rbp);
    assembler->mov(x86::rbp, x86::rsp);
    assembler->push(threadPtr);       // rbx
    assembler->push(registersPtr);    // r12
    assembler->push(jitFunctionPtr);  // r13
    assembler->push(jumpTablePtr);    // r14
    assembler->push(pcReg);           // r15

    // Shadow space + scratch + spill slot. On entry rsp was 8 off a 16-byte boundary
    // (the return address), 6 pushes later it still is, so 56 re-aligns it.
    assembler->sub(x86::rsp, FRAME_LOCALS);

    // Store parameters in callee-saved registers
    assembler->mov(threadPtr, arg0);
    assembler->mov(jitFunctionPtr, arg1);
    assembler->mov(registersPtr, arg2);

    // Load jump table address from jit_compiled_function->jumpTable
    assembler->mov(jumpTablePtr, x86::qword_ptr(jitFunctionPtr, offsetof(jit_compiled_function, jumpTable)));

    // load the current pc value, should be 0 if first time function call
    loadPC();

    /*
     * Context Switch Frame Rebuild Logic
     * ==================================
     *
     *   if(thread->task->current != fun->originalFunction) {
     *       sharp_function *frame = get_next_frame(fun->originalFunction);
     *       invoke_next_frame(frame, false);
     *       goto check_state;
     *   }
     *
     * If the function on top of the VM call stack is not us, we are being resumed
     * after a context switch and the frames above us have to be run first.
     */
    Label skipFrameRebuild = assembler->newLabel();

    loadTask(tmp0);
    assembler->mov(tmp1, x86::qword_ptr(tmp0, offsetof(fiber, current)));
    assembler->cmp(tmp1, x86::qword_ptr(jitFunctionPtr, offsetof(jit_compiled_function, originalFunction)));
    assembler->je(skipFrameRebuild);

    // frame = get_next_frame(fun->originalFunction)
    assembler->mov(arg0, x86::qword_ptr(jitFunctionPtr, offsetof(jit_compiled_function, originalFunction)));
    callFunction(reinterpret_cast<void*>(get_next_frame));

    // invoke_next_frame(frame, false)
    assembler->mov(arg0, x86::rax);
    assembler->xor_(arg1.r32(), arg1.r32());
    callFunction(reinterpret_cast<void*>(invoke_next_frame));
    loadPC();

    assembler->jmp(stateCheckLabel);

    assembler->bind(skipFrameRebuild);

    // If we are being re-entered part way through the function (task->pc != rom)
    // resume at that pc through the state check / jump table instead of
    // falling through and re-running the function from instruction 0.
    assembler->test(pcReg, pcReg);
    assembler->jnz(stateCheckLabel);

    return true;
}

bool X64Compiler::setupFunctionEpilogue() {
    generateStateCheckSection();
    generateExceptionHandlerSection();
    generateReturnSection();
    generateIllegalBranchSection();
    generateNullptrExceptionSection();
    generateStackOverflowSection();
    return true;
}

void X64Compiler::resetCodeHolder() {
    // Reset CodeHolder (detaches the old assembler) and start fresh
    code->reset(ResetPolicy::kSoft);

    if(assembler) {
        delete assembler;
        assembler = nullptr;
    }

    Environment x64Env(Arch::kX64);
    code->init(x64Env);
    errorHandler.failed = false;
    code->setErrorHandler(&errorHandler);

    assembler = new x86::Assembler(code);

    // Labels belong to the code holder, recreate them
    createLabels();
    currentFunction = nullptr;
}

// Helper functions and section implementations are in separate files
