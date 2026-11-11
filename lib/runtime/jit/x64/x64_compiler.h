//
// x86-64 JIT Compiler Implementation for Sharp Runtime
//

#ifndef SHARP_X64_COMPILER_H
#define SHARP_X64_COMPILER_H

#include "../jit_compiler.h"
#include "../../../util/jit/asmjit/src/asmjit/core.h"
#include "../../../util/jit/asmjit/src/asmjit/x86.h"
#include "../../../core/vm_register.h"
#include "../../../core/thread_state.h"
#include <vector>

using namespace asmjit;

// Which comparison emitCompare() should produce
enum jit_compare {
    JIT_CMP_LT,   // left <  right
    JIT_CMP_GT,   // left >  right
    JIT_CMP_LTE,  // left <= right
    JIT_CMP_GTE,  // left >= right
    JIT_CMP_EQ,   // left == right
    JIT_CMP_NE    // left != right
};

class X64Compiler : public JitCompiler {
private:
    JitRuntime runtime;
    CodeHolder *code;
    x86::Assembler *assembler;
    JitErrorHandler errorHandler;
    sharp_function *currentFunction;  // Current function being compiled

    /*
     * x86-64 Register Usage Map for JIT Compiler
     * ==========================================
     *
     * CALLEE-SAVED REGISTERS (survive calls into C++ code, on Linux/macOS and Windows):
     * - rbx (threadPtr)        : Points to Sharp thread context
     * - r12 (registersPtr)     : Points to thread-local VM registers array
     * - r13 (jitFunctionPtr)   : Points to jit_compiled_function structure
     * - r14 (jumpTablePtr)     : Points to jump table array for PC dispatch
     * - r15 (pcReg)            : Current Sharp PC (instruction index)
     * - rbp (framePtr)         : Frame pointer
     *
     * CALLER-SAVED TEMPORARIES (DESTROYED by every call into C++ code):
     * - rax (tmp0)             : General purpose temp / C function return value
     * - r10 (tmp1)             : General purpose temp
     * - r11 (tmp2)             : General purpose temp
     * - xmm0, xmm1             : Floating point temps
     * - st0..st1 (x87)         : Used for VM register (long double) access
     *
     * None of the temporaries is an argument register on either calling convention,
     * so arguments can always be loaded from them without overwriting each other.
     *
     * ARGUMENT REGISTERS (arg0, arg1, arg2):
     * - System V (Linux, macOS, WSL): rdi, rsi, rdx
     * - Windows x64 (incl. Cygwin)  : rcx, rdx, r8
     *
     * STACK FRAME LAYOUT (after the prologue, rsp is 16-byte aligned):
     * [rbp +  8]  return address
     * [rbp +  0]  saved rbp
     * [rbp -  8]  saved rbx
     * [rbp - 16]  saved r12
     * [rbp - 24]  saved r13
     * [rbp - 32]  saved r14
     * [rbp - 40]  saved r15
     * [rsp + 48]  spill slot   (8 bytes, value that must survive a C call)
     * [rsp + 32]  scratch slot (16 bytes, int <-> floating point conversions)
     * [rsp +  0]  32 bytes of shadow space (required by the Windows x64 ABI)
     */
    x86::Gp threadPtr;        // rbx
    x86::Gp registersPtr;     // r12
    x86::Gp jitFunctionPtr;   // r13
    x86::Gp jumpTablePtr;     // r14
    x86::Gp pcReg;            // r15
    x86::Gp tmp0;             // rax
    x86::Gp tmp1;             // r10
    x86::Gp tmp2;             // r11
    x86::Gp arg0, arg1, arg2; // calling convention argument registers

    static constexpr int FRAME_LOCALS  = 56;  // sub rsp, 56 (keeps rsp 16-byte aligned)
    static constexpr int SCRATCH_SLOT  = 32;  // [rsp + 32]
    static constexpr int SPILL_SLOT    = 48;  // [rsp + 48]

    // How sharp_object::type (a bit-field) is laid out by this compiler,
    // discovered at startup so it can be read without calling into C++.
    int objectTypeOffset;     // byte offset inside sharp_object
    int objectTypeMask;       // mask inside that byte
    int objectTypeShift;      // shift of the field inside that byte

    // Labels for control flow
    Label stateCheckLabel;         // thread signal check + dispatch to pcReg
    Label catchExceptionLabel;     // exception handling
    Label returnFromFunctionLabel; // centralized function return (code in eax)
    Label illegalBranchLabel;      // branch to something that is not an instruction
    Label nullptrExceptionLabel;   // throws vm.nullptr_except
    Label stackOverflowLabel;      // throws vm.stack_overflow_except

    // Jump table management
    std::vector<Label> opcodeLabels;   // Labels for each opcode (indexed by PC)
    std::vector<size_t> branchTargets; // PCs that emitBranchTo() jumps to directly (must all be real instructions)
    size_t currentPC;                  // Current PC being compiled

public:
    X64Compiler();
    ~X64Compiler() override;

    bool compileFunction(sharp_function* function, jit_compiled_function* output) override;
    void releaseCompiledFunction(jit_compiled_function* compiledFunc) override;

    // Sharp IR opcode implementations for x86-64 - All opcodes
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
    void createLabels();
    void probeObjectTypeLayout();

    // ---- memory operand helpers -------------------------------------------------
    x86::Mem vmRegister(int vmReg);                        // registers[vmReg] (long double sized)
    x86::Mem stackVar(x86::Gp base, int relAddress = 0);   // (base + relAddress)->var (double)
    x86::Mem stackObj(x86::Gp base, int relAddress = 0);   // (base + relAddress)->obj.o (pointer)
    x86::Mem scratch(uint32_t size = 8);                   // scratch slot in our native frame

    // ---- VM register helpers (x87, exact for long double) -----------------------
    void fldRegister(int vmReg);                           // push registers[vmReg] on the x87 stack
    void fstpRegister(int vmReg);                          // pop the x87 stack into registers[vmReg]

    void emitLoadInt(int value);                           // push (long double)value on the x87 stack
    void emitLoadDoubleConstant(x86::Vec xmm, double value); // xmm = value                      (uses tmp2)
    void emitRegisterToInt(int vmReg, x86::Gp dest);       // dest = (Int)registers[vmReg]      (truncates like a C++ cast)
    void emitSetIfEqual();                                 // al = 1 if the last x87 compare was "equal" (NaN is not)
    void emitStoreBool(int vmReg);                         // registers[vmReg] = al ? 1 : 0     (uses tmp0)
    void emitCompare(int leftReg, int rightReg, int destReg, jit_compare kind); // registers[dest] = left <op> right
    x86::Mem headElement(x86::Gp base, x86::Gp index);     // base[index] for a long double[] (may shift index in place)

    // ---- vm stack helpers -------------------------------------------------------
    void emitPushSlot(x86::Gp slot);                       // slot = ++task->sp                 (uses tmp0)
    void emitPopSlot(x86::Gp slot);                        // slot = task->sp--                 (uses tmp0)

    // ---- task / pc helpers ------------------------------------------------------
    void loadTask(x86::Gp dest);                           // dest = thread->task
    void storePC();                                        // task->pc = task->rom + pcReg   (uses tmp0, tmp1)
    void loadPC();                                         // pcReg = task->pc - task->rom   (uses tmp0)

    // ---- calls into C++ ---------------------------------------------------------
    void callFunction(void* functionPtr);                  // args must already be in arg0..arg2

    // ---- shared checks ----------------------------------------------------------
    void emitStateCheck(size_t offset);                    // pcReg += offset; goto state_check
    void emitAfterCall();                                  // after CALL-like opcodes: continue at task->pc or unwind
    void emitBranchTo(size_t targetPc);                    // task->pc = rom + targetPc; check_state(0)  (fast path: direct jump)
    void emitReturn(int returnCode);                       // return from the compiled function
    void emitFastExceptionCheck();                         // if(hasSignal(tsig_except)) goto catch_exception
    void emitStackCheck(int n);                            // grow_stack_for(n) + stack_overflow_check_for(n)
    void emitRequireObjectWithValue(x86::Gp objectReg);    // objectReg = task->ptr->o or throw nullptr
    void emitRequireNumericObjectWithValue(x86::Gp objectReg); // same + o->type <= type_var

    // ---- sections ---------------------------------------------------------------
    void generateStateCheckSection();
    void generateExceptionHandlerSection();
    void generateReturnSection();
    void generateIllegalBranchSection();
    void generateNullptrExceptionSection();
    void generateStackOverflowSection();

    // ---- jump table -------------------------------------------------------------
    void initializeJumpTable(size_t opcodeCount);
};

#endif //SHARP_X64_COMPILER_H
