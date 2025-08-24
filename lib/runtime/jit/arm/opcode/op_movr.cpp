//
// ARM64 JIT Compiler - MOVR Opcode Implementation
//

#include "../arm64_compiler.h"

using namespace asmjit;

bool Arm64Compiler::emit_movr(int outRegister, int inRegister) {
    /*
     * VM Interpreter Code:
     * MOVR:
     *     regs[dual_arg1] = regs[dual_arg2];
     *     branch
     */
    
    // TODO: Implement MOVR opcode
    // registers[outRegister] = registers[inRegister]
    return false;
}