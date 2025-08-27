//
// ARM64 JIT Compiler Implementation for Sharp Runtime
//

#ifndef SHARP_ARM64_COMPILER_H
#define SHARP_ARM64_COMPILER_H

#include "../jit_compiler.h"
#include "../../../util/jit/asmjit/src/asmjit/core.h"
#include "../../../util/jit/asmjit/src/asmjit/a64.h"
#include "../../../core/vm_register.h"
#include "../../../core/thread_state.h"
#include <vector>

using namespace asmjit;

class Arm64Compiler : public JitCompiler {
private:
    JitRuntime runtime;
    CodeHolder code;
    a64::Assembler assembler;
    
    /*
     * ARM64 Register Usage Map for JIT Compiler
     * ==========================================
     * 
     * CALLEE-SAVED REGISTERS (must be preserved across function calls):
     * - x19 (threadPtr)        : Points to Sharp thread context
     * - x20 (registersPtr)     : Points to thread-local VM registers array  
     * - x21 (tempReg1)         : General purpose temporary register
     * - x22 (tempReg2)         : General purpose temporary register
     * - x23 (tempReg3)         : General purpose temporary register
     * - x24 (jumpTablePtr)     : Points to jump table array for PC dispatch
     * - x25 (jitFunctionPtr)   : Points to jit_compiled_function structure
     * - x26 (tempReg4)         : General purpose temporary register
     * - d8  (tempVec1)         : Vector register for long double operations
     * - d9  (tempVec2)         : Vector register for long double operations
     * 
     * CALLER-SAVED REGISTERS (used temporarily, not preserved):
     * - x0  (returnReg)        : Function parameter 1 (sharp_thread*), return value
     * - x1                     : Function parameter 2 (jit_compiled_function*)
     * - x2                     : Function parameter 3 (long double* registers)
     * - x29 (framePtr)         : Frame pointer (saved/restored by prologue/epilogue)
     * - x30 (linkReg)          : Link register (saved/restored by prologue/epilogue)
     * - sp  (stackPtr)         : Stack pointer
     * 
     * STACK FRAME LAYOUT:
     * [High Address]
     * +16: x29, x30 (frame pointer, link register)
     * +0:  x19, x20 (threadPtr, registersPtr)  
     * -16: x21, x22 (tempReg1, tempReg2)
     * -32: x23, x24 (tempReg3, jumpTablePtr)
     * -48: x25, x26 (jitFunctionPtr, tempReg4)
     * -64: d8, d9   (tempVec1, tempVec2)
     * -80: [reserved stack space]
     * [Low Address]
     * 
     * NOTE: When adding new registers, update both prologue/epilogue save/restore code!
     */
     
    // Labeled ARM64 registers for JIT operations
    a64::Gp threadPtr;        // x19 - Points to Sharp thread context
    a64::Gp registersPtr;     // x20 - Points to thread-local registers array
    a64::Gp jitFunctionPtr;   // x25 - Points to jit_compiled_function structure
    a64::Vec tempVec1;        // d8  - Floating point temp register for long double operations
    a64::Vec tempVec2;        // d9  - Floating point temp register for long double operations  
    a64::Gp tempReg1;         // x21 - General purpose temp register
    a64::Gp tempReg2;         // x22 - General purpose temp register  
    a64::Gp tempReg3;         // x23 - General purpose temp register
    a64::Gp tempReg4;         // x26 - General purpose temp register
    a64::Gp jumpTablePtr;     // x24 - Pointer to jump table array
    a64::Gp pcReg;            // x27 - Current PC register (tracks current Sharp PC during compilation)
    
    // Standard ARM64 registers (for clarity and consistency)
    a64::Gp returnReg;        // w0/x0 - Function return value register
    a64::Gp framePtr;         // x29 - Frame pointer
    a64::Gp linkReg;          // x30 - Link register  
    a64::Gp stackPtr;         // sp - Stack pointer
    
    // Labels for control flow
    Label stateCheckLabel;    // Label for state check section
    Label catchExceptionLabel; // Label for exception handling
    Label returnFromFunctionLabel; // Label for centralized function return
    Label growStackLabel;     // Label for centralized grow stack section
    Label stackOverflowLabel; // Label for centralized stack overflow section
    
    // Jump table management
    std::vector<Label> opcodeLabels;   // Labels for each opcode (indexed by PC)
    size_t currentPC;                  // Current PC being compiled (for state check returns)
    
public:
    Arm64Compiler();
    ~Arm64Compiler() override;
    
    bool compileFunction(sharp_function* function, jit_compiled_function* output) override;
    void releaseCompiledFunction(jit_compiled_function* compiledFunc) override;
    
    // Sharp IR opcode implementations for ARM64 - All opcodes
    bool emit_nop() override;
    bool emit_int(int flag) override;
    bool emit_movi(int value, int outRegister) override;
    bool emit_ret(int errState) override;
    bool emit_hlt() override;
    bool emit_newarray(int inRegister, int ntype) override;
    bool emit_cast(int classAddress) override;
    bool emit_newclass(int address) override;
    bool emit_newstring(int address) override;
    bool emit_newobjarray(int inRegister) override;
    bool emit_newclassarray(int inRegister, int classAddress) override;
    bool emit_del() override;
    bool emit_pushobj() override;
    bool emit_popobj() override;
    bool emit_popobj_2() override;
    bool emit_mov8(int outRegister, int registerToCast) override;
    bool emit_mov16(int outRegister, int registerToCast) override;
    bool emit_mov32(int outRegister, int registerToCast) override;
    bool emit_mov64(int outRegister, int registerToCast) override;
    bool emit_movu8(int outRegister, int registerToCast) override;
    bool emit_movu16(int outRegister, int registerToCast) override;
    bool emit_movu32(int outRegister, int registerToCast) override;
    bool emit_movu64(int outRegister, int registerToCast) override;
    bool emit_movr(int outRegister, int inRegister) override;
    bool emit_movl(int relFrameAddress) override;
    bool emit_movsl(int relStackAddress) override;
    bool emit_movn(int address) override;
    bool emit_movg(int address) override;
    bool emit_movnd(int inRegister) override;
    bool emit_movabs(int address) override;
    bool emit_loadabs(int address) override;
    bool emit_rmov(int indexRegister, int inRegister) override;
    bool emit_smov(int outRegister, int relStackAddress) override;
    bool emit_smovr(int inRegister, int relStackAddress) override;
    bool emit_smovr_2(int inRegister, int relFrameAddress) override;
    bool emit_smovr_3(int relFrameAddress) override;
    bool emit_smovr_4(int relFrameAddress1, int relFrameAddress2) override;
    bool emit_imov(int inRegister) override;
    bool emit_rstore(int registerToCast) override;
    bool emit_istore(int value) override;
    bool emit_istorel(int relFrameAddress, int integerValue) override;
    bool emit_loadl(int outRegister, int relFrameAddress) override;
    bool emit_loadval(int outRegister) override;
    bool emit_loadpc(int outRegister) override;
    bool emit_iaload(int outRegister, int indexRegister) override;
    bool emit_iload(int outRegister) override;
    bool emit_ldc(int outRegister, int address) override;
    bool emit_add(int outRegister, int leftRegister, int rightRegister) override;
    bool emit_sub(int outRegister, int leftRegister, int rightRegister) override;
    bool emit_mul(int outRegister, int leftRegister, int rightRegister) override;
    bool emit_div(int outRegister, int leftRegister, int rightRegister) override;
    bool emit_mod(int outRegister, int leftRegister, int rightRegister) override;
    bool emit_iadd(int outRegister, int value) override;
    bool emit_isub(int outRegister, int value) override;
    bool emit_imul(int outRegister, int value) override;
    bool emit_idiv(int outRegister, int value) override;
    bool emit_imod(int outRegister, int value) override;
    bool emit_addl(int inRegister, int relFrameAddress) override;
    bool emit_subl(int inRegister, int relFrameAddress) override;
    bool emit_mull(int inRegister, int relFrameAddress) override;
    bool emit_divl(int inRegister, int relFrameAddress) override;
    bool emit_modl(int inRegister, int relFrameAddress) override;
    bool emit_iaddl(int value, int relFrameAddress) override;
    bool emit_isubl(int value, int relFrameAddress) override;
    bool emit_imull(int value, int relFrameAddress) override;
    bool emit_idivl(int value, int relFrameAddress) override;
    bool emit_imodl(int value, int relFrameAddress) override;
    bool emit_inc(int outRegister) override;
    bool emit_dec(int outRegister) override;
    bool emit_neg(int outRegister, int inRegister) override;
    bool emit_exp(int outRegister, int leftRegister, int rightRegister) override;
    bool emit_isadd(int relStackAddress, int value) override;
    bool emit_and(int leftRegister, int rightRegister) override;
    bool emit_uand(int leftRegister, int rightRegister) override;
    bool emit_or(int leftRegister, int rightRegister) override;
    bool emit_xor(int leftRegister, int rightRegister) override;
    bool emit_andl(int inRegister, int relStackAddress) override;
    bool emit_orl(int inRegister, int relStackAddress) override;
    bool emit_xorl(int inRegister, int relStackAddress) override;
    bool emit_shl(int outRegister, int leftRegister, int rightRegister) override;
    bool emit_shr(int outRegister, int leftRegister, int rightRegister) override;
    bool emit_not(int outRegister, int inRegister) override;
    bool emit_pop() override;
    bool emit_popl(int relFrameAddress) override;
    bool emit_ipopl(int relFrameAddress) override;
    bool emit_pushnull() override;
    bool emit_ipushl(int relFrameAddress) override;
    bool emit_pushl(int relFrameAddress) override;
    bool emit_dup() override;
    bool emit_swap() override;
    bool emit_lt(int leftRegister, int rightRegister) override;
    bool emit_gt(int leftRegister, int rightRegister) override;
    bool emit_lte(int leftRegister, int rightRegister) override;
    bool emit_gte(int leftRegister, int rightRegister) override;
    bool emit_test(int leftRegister, int rightRegister) override;
    bool emit_tne(int leftRegister, int rightRegister) override;
    bool emit_itest(int outRegister) override;
    bool emit_cmp(int inRegister, int value) override;
    bool emit_is(int outRegister, int type) override;
    bool emit_jmp(int address) override;
    bool emit_je(int address) override;
    bool emit_jne(int address) override;
    bool emit_ife() override;
    bool emit_ifne() override;
    bool emit_brh() override;
    bool emit_skip(int instructionsToSkip) override;
    bool emit_skpe(int inRegister, int address) override;
    bool emit_skne(int inRegister, int address) override;
    bool emit_call(int address) override;
    bool emit_calld(int inRegister) override;
    bool emit_invoke_delegate(int address, int argCount, bool isFunctionStatic) override;
    bool emit_returnval(int inRegister) override;
    bool emit_returnobj() override;
    bool emit_sizeof(int outRegister) override;
    bool emit_put(int inRegister) override;
    bool emit_putc(int inRegister) override;
    bool emit_checklen(int inRegister) override;
    bool emit_checknull(int outRegister) override;
    bool emit_get(int outRegister) override;
    bool emit_sleep(int inRegister) override;
    bool emit_lock() override;
    bool emit_ulock() override;
    bool emit_throw() override;
    bool emit_varcast(int varType, bool isArray) override;
    bool emit_tls_movl(int address) override;
    
protected:
    bool setupFunctionPrologue() override;
    bool setupFunctionEpilogue() override;
    bool translateOpcode(uint32_t opcode, uint32_t** pc, sharp_function* function) override;
    
private:
    void resetCodeHolder();
    
    // High-level helper functions for VM register operations
    void loadRegisterValue(a64::Vec dest, _register vmReg);          // Load registers[vmReg] into dest
    void storeRegisterValue(_register vmReg, a64::Vec src);          // Store src into registers[vmReg]
    void addRegisters(_register destReg, _register leftReg, _register rightReg);  // registers[destReg] = registers[leftReg] + registers[rightReg]
    void subRegisters(_register destReg, _register leftReg, _register rightReg);  // registers[destReg] = registers[leftReg] - registers[rightReg]
    void mulRegisters(_register destReg, _register leftReg, _register rightReg);  // registers[destReg] = registers[leftReg] * registers[rightReg]
    void divRegisters(_register destReg, _register leftReg, _register rightReg);  // registers[destReg] = registers[leftReg] / registers[rightReg]
    void moveRegister(_register destReg, _register srcReg);          // registers[destReg] = registers[srcReg]
    void setRegisterImmediate(_register vmReg, int64_t value);       // registers[vmReg] = immediate value
    
    // Stack operation helpers
    void popStackNumber(a64::Vec destVec);                           // Pop numeric value from stack: (task->sp--)->var
    void pushStackNumber(a64::Vec srcVec);                           // Push numeric value to stack: (++task->sp)->var
    
    // State checking helpers
    void emitStateCheck(size_t offset);                             // Emit jump to state check with PC offset
    void emitStateCheckNext();                                       // Emit state check, resume at next instruction (currentPC + 1)
    void generateStateCheckSection();                                // Generate the state check code section
    
    // Return helpers
    void emitReturn(int returnCode = 0);                             // Jump to centralized return with immediate return code (uses currentPC)
    void generateReturnSection();                                    // Generate the centralized return section
    
    // External function call helpers
    void callStaticFunction(void* functionPtr);                      // Call external C function with no parameters
    void callStaticFunction(void* functionPtr, a64::Gp param1, a64::Gp param2); // Call external C function with 2 parameters
    void callInstanceFunction(void* functionPtr, a64::Gp instance, a64::Gp param1); // Call C++ instance method with 1 parameter
    
    // Jump table helpers
    void initializeJumpTable(size_t opcodeCount);                    // Initialize jump table for function
    void setJumpTableEntry(size_t pc, Label opcodeLabel);            // Set jump table entry for PC
    void generateJumpDispatch(int targetPCRegister);                 // Generate jump to PC using jump table
    
    // PC management helpers
    void storePC();                                                  // Store pcReg value to task->pc
    void loadPC();                                                   // Load numeric PC from task->pc to pcReg
    
    // Exception handling helpers
    void emitExceptionHandle();                                      // Jump to exception handler
    void generateExceptionHandlerSection();                          // Generate exception handling code section
    
    // VM Stack Operation Helpers - Centralized Sections
    void emitGrowStackCheck(int n, Label returnLabel);               // Jump to grow stack section with return label
    void emitStackOverflowCheck(int n, Label returnLabel);               // Jump to stack overflow section
    void generateGrowStackSection();                                 // Generate centralized grow stack section
    void generateStackOverflowSection();                             // Generate centralized stack overflow section
    
    // VM Stack Operation Helpers - Direct Operations
    void emitPushStackNumber(double value);                          // VM macro: push_stack_number = value
    void emitPushStackNumberImmediate(int32_t intValue);             // VM macro: push_stack_number = raw_arg2
};

#endif //SHARP_ARM64_COMPILER_H