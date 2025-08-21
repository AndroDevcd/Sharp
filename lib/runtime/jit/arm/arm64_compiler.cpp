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
#include <cstddef>  // For offsetof
#include "helpers.cpp"
#include "sections/state_check_section.cpp"
#include "sections/state_check_helpers.cpp"
#include "sections/return_section.cpp"
#include "sections/exception_handler_section.cpp"

using namespace asmjit;

Arm64Compiler::Arm64Compiler() : assembler(&code) {
    code.init(runtime.environment(), runtime.cpuFeatures());
    
    // Initialize labeled ARM64 registers
    threadPtr = a64::x19;     // Callee-saved register for thread context pointer
    registersPtr = a64::x20;  // Callee-saved register for registers array pointer
    tempVec1 = a64::d8;       // Callee-saved vector register 1 for floating point
    tempVec2 = a64::d9;       // Callee-saved vector register 2 for floating point
    tempReg1 = a64::x21;      // Callee-saved temp register 1
    tempReg2 = a64::x22;      // Callee-saved temp register 2
    tempReg3 = a64::x23;      // Callee-saved temp register 3
    
    // Initialize standard ARM64 registers for consistency
    returnReg = a64::w0;      // Function return value register (32-bit)
    framePtr = a64::x29;      // Frame pointer
    linkReg = a64::x30;       // Link register
    stackPtr = a64::sp;       // Stack pointer
    
    // Initialize labels
    stateCheckLabel = assembler.newLabel();
    catchExceptionLabel = assembler.newLabel();
    returnFromFunctionLabel = assembler.newLabel();
    continueLabels.clear();
}

Arm64Compiler::~Arm64Compiler() {
    
}

bool Arm64Compiler::compileFunction(sharp_function* function, jit_compiled_function* output) {
    if(!should_compile_function(function)) {
        return false;
    }
    
    resetCodeHolder();
    
    if(!setupFunctionPrologue()) {
        return false;
    }
    
    // Translate Sharp IR opcodes to ARM64 assembly
    uint32_t* pc = function->bytecode;
    uint32_t* endPc = pc + function->bytecodeSize;
    
    while(pc < endPc) {
        if(!translateOpcode(*pc, pc, function)) {
            return false;
        }
        pc++;
    }
    
    if(!setupFunctionEpilogue()) {
        return false;
    }
    
    // Finalize and get the compiled code
    void* func;
    Error err = runtime.add(&func, &code);
    if(err) {
        return false;
    }
    
    output->compiledCode = reinterpret_cast<jit_function_ptr>(func);
    output->codeSize = code.codeSize();
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

bool Arm64Compiler::translateOpcode(uint32_t opcode, uint32_t* pc, sharp_function* function) {
    uint8_t op = GET_OP(opcode);
    
    switch(op) {
        // Basic opcodes
        case Opcode::NOP:
            return emit_nop();
        case Opcode::INT:
            return emit_int(GET_Da(opcode));
        case Opcode::MOVI:
            return emit_movi(*(pc + 1), GET_Da(opcode));
        case Opcode::RET:
            return emit_ret(GET_Da(opcode));
        case Opcode::HLT:
            return emit_hlt();
            
        // Array and object operations
        case Opcode::NEWARRAY:
            return emit_newarray(GET_Ca(opcode), GET_Cb(opcode));
        case Opcode::CAST:
            return emit_cast(GET_Da(opcode));
        case Opcode::NEWCLASS:
            return emit_newclass(*(pc + 1));
        case Opcode::NEWSTRING:
            return emit_newstring(GET_Da(opcode));
        case Opcode::NEWOBJARRAY:
            return emit_newobjarray(GET_Da(opcode));
        case Opcode::NEWCLASSARRAY:
            return emit_newclassarray(GET_Ca(opcode), *(pc + 1));
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
        case Opcode::MOVN:
            return emit_movn(*(pc + 1));
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
        case Opcode::SMOVR_4:
            return emit_smovr_4(GET_Ca(*(pc + 1)), GET_Cb(*(pc + 1)));
        case Opcode::IMOV:
            return emit_imov(GET_Da(opcode));
            
        // Store and load operations
        case Opcode::RSTORE:
            return emit_rstore(GET_Da(opcode));
        case Opcode::ISTORE:
            return emit_istore(*(pc + 1));
        case Opcode::ISTOREL:
            return emit_istorel(GET_Ca(*(pc + 1)), GET_Cb(*(pc + 1)));
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
        case Opcode::IADD:
            return emit_iadd(GET_Ca(opcode), *(pc + 1));
        case Opcode::ISUB:
            return emit_isub(GET_Ca(opcode), *(pc + 1));
        case Opcode::IMUL:
            return emit_imul(GET_Ca(opcode), *(pc + 1));
        case Opcode::IDIV:
            return emit_idiv(GET_Ca(opcode), *(pc + 1));
        case Opcode::IMOD:
            return emit_imod(GET_Ca(opcode), *(pc + 1));
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
        case Opcode::IADDL:
            return emit_iaddl(*(pc + 1), GET_Da(opcode));
        case Opcode::ISUBL:
            return emit_isubl(*(pc + 1), GET_Da(opcode));
        case Opcode::IMULL:
            return emit_imull(*(pc + 1), GET_Da(opcode));
        case Opcode::IDIVL:
            return emit_idivl(*(pc + 1), GET_Da(opcode));
        case Opcode::IMODL:
            return emit_imodl(*(pc + 1), GET_Da(opcode));
        case Opcode::INC:
            return emit_inc(GET_Da(opcode));
        case Opcode::DEC:
            return emit_dec(GET_Da(opcode));
        case Opcode::NEG:
            return emit_neg(GET_Ca(opcode), GET_Cb(opcode));
        case Opcode::EXP:
            return emit_exp(GET_Ba(opcode), GET_Bb(opcode), GET_Bc(opcode));
        case Opcode::ISADD:
            return emit_isadd(GET_Ca(*(pc + 1)), GET_Cb(*(pc + 1)));
            
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
        case Opcode::CMP:
            return emit_cmp(GET_Ca(opcode), *(pc + 1));
        case Opcode::IS:
            return emit_is(GET_Ca(opcode), *(pc + 1));
            
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
        case Opcode::INVOKE_DELEGATE:
            return emit_invoke_delegate(GET_Ca(*(pc + 1)), GET_Cb(*(pc + 1)), GET_Ca(opcode));
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
    // JIT function signature: int jit_func(sharp_thread* thread, long double* registers)
    // x0 = sharp_thread* thread
    // x1 = long double* registers
    
    // Save frame pointer and link register
    assembler.stp(framePtr, linkReg, a64::ptr(stackPtr, -16).pre());
    
    // Set up frame pointer
    assembler.mov(framePtr, stackPtr);
    
    // Save all callee-saved registers we use (ARM64 requires this)
    // Save general purpose callee-saved registers: x19, x20, x21, x22, x23
    assembler.stp(threadPtr, registersPtr, a64::ptr(stackPtr, -16).pre());
    assembler.stp(tempReg1, tempReg2, a64::ptr(stackPtr, -16).pre());
    assembler.str(tempReg3, a64::ptr(stackPtr, -16).pre());
    
    // Save vector callee-saved registers: d8, d9
    assembler.stp(tempVec1, tempVec2, a64::ptr(stackPtr, -16).pre());
    
    // Reserve minimal stack space (16 bytes for alignment)
    assembler.sub(stackPtr, stackPtr, 16);
    
    // Store parameters in callee-saved registers
    assembler.mov(threadPtr, a64::x0);      // threadPtr = sharp_thread* (first parameter)
    assembler.mov(registersPtr, a64::x1);   // registersPtr = long double* registers (second parameter)
    
    return true;
}

bool Arm64Compiler::setupFunctionEpilogue() {
    // Generate state check section before epilogue
    generateStateCheckSection();
    
    // Generate exception handler section
    generateExceptionHandlerSection();
    
    // Generate centralized return section
    generateReturnSection();
    
    return true;
}

void Arm64Compiler::resetCodeHolder() {
    code.reset();
    code.init(runtime.environment(), runtime.cpuFeatures());
    
    // Reinitialize labels after reset
    stateCheckLabel = assembler.newLabel();
    catchExceptionLabel = assembler.newLabel();
    returnFromFunctionLabel = assembler.newLabel();
    continueLabels.clear();
}

// Helper functions and section implementations are now in separate files

