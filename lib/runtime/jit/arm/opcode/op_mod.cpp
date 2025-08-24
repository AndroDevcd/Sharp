//
// ARM64 JIT Compiler - MOD Opcode Implementation
//

#include "../arm64_compiler.h"

using namespace asmjit;

bool Arm64Compiler::emit_mod(int outRegister, int leftRegister, int rightRegister) {
    /*
     * VM Interpreter Code:
     * MOD:
     *     regs[triple_arg3] = (Int)regs[triple_arg1] % (Int)regs[triple_arg2];
     *     branch
     */
    
    // TODO: Implement MOD opcode
    return false;
}