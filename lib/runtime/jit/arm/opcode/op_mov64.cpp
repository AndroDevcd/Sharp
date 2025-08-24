//
// ARM64 JIT Compiler - MOV64 Opcode Implementation
//

#include "../arm64_compiler.h"

using namespace asmjit;

bool Arm64Compiler::emit_mov64(int outRegister, int registerToCast) {
    /*
     * VM Interpreter Code:
     * MOV64:
     *     regs[dual_arg1] = (int64_t)regs[dual_arg2];
     *     branch
     */
    
    // TODO: Implement MOV64 opcode
    return false;
}