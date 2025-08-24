//
// ARM64 JIT Compiler - MOVU8 Opcode Implementation
//

#include "../arm64_compiler.h"

using namespace asmjit;

bool Arm64Compiler::emit_movu8(int outRegister, int registerToCast) {
    /*
     * VM Interpreter Code:
     * MOVU8:
     *     regs[dual_arg1] = (uint8_t)regs[dual_arg2];
     *     branch
     */
    
    // TODO: Implement MOVU8 opcode
    return false;
}