//
// ARM64 JIT Compiler - MOV8 Opcode Implementation
//

#include "../arm64_compiler.h"

using namespace asmjit;

bool Arm64Compiler::emit_mov8(int outRegister, int registerToCast) {
    /*
     * VM Interpreter Code:
     * MOV8:
     *     regs[dual_arg1] = (int8_t)regs[dual_arg2];
     *     branch
     */
    
    // TODO: Implement MOV8 opcode
    return false;
}