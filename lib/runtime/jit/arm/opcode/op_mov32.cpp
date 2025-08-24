//
// ARM64 JIT Compiler - MOV32 Opcode Implementation
//

#include "../arm64_compiler.h"

using namespace asmjit;

bool Arm64Compiler::emit_mov32(int outRegister, int registerToCast) {
    /*
     * VM Interpreter Code:
     * MOV32:
     *     regs[dual_arg1] = (int32_t)regs[dual_arg2];
     *     branch
     */
    
    // TODO: Implement MOV32 opcode
    return false;
}