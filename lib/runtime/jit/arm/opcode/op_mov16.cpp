//
// ARM64 JIT Compiler - MOV16 Opcode Implementation
//

#include "../arm64_compiler.h"

using namespace asmjit;

bool Arm64Compiler::emit_mov16(int outRegister, int registerToCast) {
    /*
     * VM Interpreter Code:
     * MOV16:
     *     regs[dual_arg1] = (int16_t)regs[dual_arg2];
     *     branch
     */
    
    // TODO: Implement MOV16 opcode
    return false;
}