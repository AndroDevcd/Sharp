//
// ARM64 JIT Compiler - MOVU64 Opcode Implementation
//

#include "../arm64_compiler.h"

using namespace asmjit;

bool Arm64Compiler::emit_movu64(int outRegister, int registerToCast) {
    /*
     * VM Interpreter Code:
     * MOVU64:
     *     regs[dual_arg1] = (uint64_t)regs[dual_arg2];
     *     branch
     */
    
    // TODO: Implement MOVU64 opcode
    return false;
}