//
// ARM64 JIT Compiler - MOVU32 Opcode Implementation
//

#include "../arm64_compiler.h"

using namespace asmjit;

bool Arm64Compiler::emit_movu32(int outRegister, int registerToCast) {
    /*
     * VM Interpreter Code:
     * MOVU32:
     *     regs[dual_arg1] = (uint32_t)regs[dual_arg2];
     *     branch
     */
    
    // TODO: Implement MOVU32 opcode
    return false;
}