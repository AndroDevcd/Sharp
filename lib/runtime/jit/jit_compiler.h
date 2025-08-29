//
// JIT Compiler Base Class for Sharp Runtime
//

#ifndef SHARP_JIT_COMPILER_H
#define SHARP_JIT_COMPILER_H

#include "../../../stdimports.h"
#include "../types/sharp_function.h"
#include "../../util/jit/asmjit/src/asmjit/core.h"

// Forward declaration
struct sharp_thread;
struct jit_compiled_function;

// JIT function return codes
enum jit_return_code {
    JIT_OK = 0,                // Function executed successfully
    JIT_EXCEPTION = 0x001,         // Exception occurred
    JIT_CONTEXT_SWITCH = 0x002,    // Context switch required
    JIT_KILL = 0x004               // Thread termination requested
};

// JIT function signature typedef
// Parameters passed to compiled JIT functions (in this order):
// 1. thread: pointer to Sharp thread context (contains all VM state)
// 2. fun: pointer to jit_compiled_function (contains jumpTable for PC dispatch)
// 3. registers: pointer to thread-local VM registers array
// Returns: jit_return_code indicating execution result
typedef int (*jit_function_ptr)(sharp_thread* thread, jit_compiled_function* fun, long double* registers);

struct jit_compiled_function {
    jit_function_ptr compiledCode;  // Typed function pointer instead of void*
    void** jumpTable;               // Array of instruction addresses indexed by PC
    size_t jumpTableSize;           // Number of entries in jump table
    size_t codeSize;
    bool isCompiled;
    sharp_function* originalFunction;
};

enum jit_target_arch {
    JIT_ARM64,
    JIT_X86_64,
    JIT_RISCV64
};

class JitCompiler {
public:
    virtual ~JitCompiler() = default;

    virtual bool compileFunction(sharp_function* function, jit_compiled_function* output) = 0;
    virtual void releaseCompiledFunction(jit_compiled_function* compiledFunc) = 0;
    
    // Virtual methods for all Sharp IR opcodes
    virtual bool emit_nop() = 0;
    virtual bool emit_int(int flag) = 0;
    virtual bool emit_movi(int value, int outRegister) = 0;
    virtual bool emit_ret(int errState) = 0;
    virtual bool emit_hlt() = 0;
    virtual bool emit_newarray(int inRegister, int ntype) = 0;
    virtual bool emit_cast(int classAddress) = 0;
    virtual bool emit_newclass(int address) = 0;
    virtual bool emit_newstring(int address) = 0;
    virtual bool emit_newobjarray(int inRegister) = 0;
    virtual bool emit_newclassarray(int inRegister, int classAddress) = 0;
    virtual bool emit_del() = 0;
    virtual bool emit_pushobj() = 0;
    virtual bool emit_popobj() = 0;
    virtual bool emit_popobj_2() = 0;
    virtual bool emit_mov8(int outRegister, int registerToCast) = 0;
    virtual bool emit_mov16(int outRegister, int registerToCast) = 0;
    virtual bool emit_mov32(int outRegister, int registerToCast) = 0;
    virtual bool emit_mov64(int outRegister, int registerToCast) = 0;
    virtual bool emit_movu8(int outRegister, int registerToCast) = 0;
    virtual bool emit_movu16(int outRegister, int registerToCast) = 0;
    virtual bool emit_movu32(int outRegister, int registerToCast) = 0;
    virtual bool emit_movu64(int outRegister, int registerToCast) = 0;
    virtual bool emit_movr(int outRegister, int inRegister) = 0;
    virtual bool emit_movl(int relFrameAddress) = 0;
    virtual bool emit_movsl(int relStackAddress) = 0;
    virtual bool emit_movn(int address) = 0;
    virtual bool emit_movg(int address) = 0;
    virtual bool emit_movnd(int inRegister) = 0;
    virtual bool emit_movabs(int address) = 0;
    virtual bool emit_loadabs(int address) = 0;
    virtual bool emit_rmov(int indexRegister, int inRegister) = 0;
    virtual bool emit_smov(int outRegister, int relStackAddress) = 0;
    virtual bool emit_smovr(int inRegister, int relStackAddress) = 0;
    virtual bool emit_smovr_2(int inRegister, int relFrameAddress) = 0;
    virtual bool emit_smovr_3(int relFrameAddress) = 0;
    virtual bool emit_smovr_4(int relFrameAddress1, int relFrameAddress2) = 0;
    virtual bool emit_imov(int inRegister) = 0;
    virtual bool emit_rstore(int registerToCast) = 0;
    virtual bool emit_istore(int value) = 0;
    virtual bool emit_istorel(int relFrameAddress, int integerValue) = 0;
    virtual bool emit_loadl(int outRegister, int relFrameAddress) = 0;
    virtual bool emit_loadval(int outRegister) = 0;
    virtual bool emit_loadpc(int outRegister) = 0;
    virtual bool emit_iaload(int outRegister, int indexRegister) = 0;
    virtual bool emit_iload(int outRegister) = 0;
    virtual bool emit_ldc(int outRegister, int address) = 0;
    virtual bool emit_add(int outRegister, int leftRegister, int rightRegister) = 0;
    virtual bool emit_sub(int outRegister, int leftRegister, int rightRegister) = 0;
    virtual bool emit_mul(int outRegister, int leftRegister, int rightRegister) = 0;
    virtual bool emit_div(int outRegister, int leftRegister, int rightRegister) = 0;
    virtual bool emit_mod(int outRegister, int leftRegister, int rightRegister) = 0;
    virtual bool emit_iadd(int outRegister, int value) = 0;
    virtual bool emit_isub(int outRegister, int value) = 0;
    virtual bool emit_imul(int outRegister, int value) = 0;
    virtual bool emit_idiv(int outRegister, int value) = 0;
    virtual bool emit_imod(int outRegister, int value) = 0;
    virtual bool emit_addl(int inRegister, int relFrameAddress) = 0;
    virtual bool emit_subl(int inRegister, int relFrameAddress) = 0;
    virtual bool emit_mull(int inRegister, int relFrameAddress) = 0;
    virtual bool emit_divl(int inRegister, int relFrameAddress) = 0;
    virtual bool emit_modl(int inRegister, int relFrameAddress) = 0;
    virtual bool emit_iaddl(int value, int relFrameAddress) = 0;
    virtual bool emit_isubl(int value, int relFrameAddress) = 0;
    virtual bool emit_imull(int value, int relFrameAddress) = 0;
    virtual bool emit_idivl(int value, int relFrameAddress) = 0;
    virtual bool emit_imodl(int value, int relFrameAddress) = 0;
    virtual bool emit_inc(int outRegister) = 0;
    virtual bool emit_dec(int outRegister) = 0;
    virtual bool emit_neg(int outRegister, int inRegister) = 0;
    virtual bool emit_exp(int outRegister, int leftRegister, int rightRegister) = 0;
    virtual bool emit_isadd(int relStackAddress, int value) = 0;
    virtual bool emit_and(int leftRegister, int rightRegister) = 0;
    virtual bool emit_uand(int leftRegister, int rightRegister) = 0;
    virtual bool emit_or(int leftRegister, int rightRegister) = 0;
    virtual bool emit_xor(int leftRegister, int rightRegister) = 0;
    virtual bool emit_andl(int inRegister, int relStackAddress) = 0;
    virtual bool emit_orl(int inRegister, int relStackAddress) = 0;
    virtual bool emit_xorl(int inRegister, int relStackAddress) = 0;
    virtual bool emit_shl(int outRegister, int leftRegister, int rightRegister) = 0;
    virtual bool emit_shr(int outRegister, int leftRegister, int rightRegister) = 0;
    virtual bool emit_not(int outRegister, int inRegister) = 0;
    virtual bool emit_pop() = 0;
    virtual bool emit_popl(int relFrameAddress) = 0;
    virtual bool emit_ipopl(int relFrameAddress) = 0;
    virtual bool emit_pushnull() = 0;
    virtual bool emit_ipushl(int relFrameAddress) = 0;
    virtual bool emit_pushl(int relFrameAddress) = 0;
    virtual bool emit_dup() = 0;
    virtual bool emit_swap() = 0;
    virtual bool emit_lt(int leftRegister, int rightRegister) = 0;
    virtual bool emit_gt(int leftRegister, int rightRegister) = 0;
    virtual bool emit_lte(int leftRegister, int rightRegister) = 0;
    virtual bool emit_gte(int leftRegister, int rightRegister) = 0;
    virtual bool emit_test(int leftRegister, int rightRegister) = 0;
    virtual bool emit_tne(int leftRegister, int rightRegister) = 0;
    virtual bool emit_itest(int outRegister) = 0;
    virtual bool emit_cmp(int inRegister, int value) = 0;
    virtual bool emit_is(int outRegister, int type) = 0;
    virtual bool emit_jmp(int address) = 0;
    virtual bool emit_je(int address) = 0;
    virtual bool emit_jne(int address) = 0;
    virtual bool emit_ife() = 0;
    virtual bool emit_ifne() = 0;
    virtual bool emit_brh() = 0;
    virtual bool emit_skip(int instructionsToSkip) = 0;
    virtual bool emit_skpe(int inRegister, int address) = 0;
    virtual bool emit_skne(int inRegister, int address) = 0;
    virtual bool emit_call(int address) = 0;
    virtual bool emit_calld(int inRegister) = 0;
    virtual bool emit_invoke_delegate(int address, int argCount, bool isFunctionStatic) = 0;
    virtual bool emit_returnval(int inRegister) = 0;
    virtual bool emit_returnobj() = 0;
    virtual bool emit_sizeof(int outRegister) = 0;
    virtual bool emit_put(int inRegister) = 0;
    virtual bool emit_putc(int inRegister) = 0;
    virtual bool emit_checklen(int inRegister) = 0;
    virtual bool emit_checknull(int outRegister) = 0;
    virtual bool emit_get(int outRegister) = 0;
    virtual bool emit_sleep(int inRegister) = 0;
    virtual bool emit_lock() = 0;
    virtual bool emit_ulock() = 0;
    virtual bool emit_throw() = 0;
    virtual bool emit_varcast(int varType, bool isArray) = 0;
    virtual bool emit_tls_movl(int address) = 0;
    
protected:
    virtual bool setupFunctionPrologue() = 0;
    virtual bool setupFunctionEpilogue() = 0;
    virtual bool translateOpcode(uint32_t opcode, uint32_t** pc, sharp_function* function) = 0;
};

extern JitCompiler* jitCompiler;
extern jit_target_arch arch;
extern recursive_mutex jit_mutex;

void init_jit_compiler();
void shutdown_jit_compiler();
bool should_compile_function(sharp_function* function);

#endif //SHARP_JIT_COMPILER_H