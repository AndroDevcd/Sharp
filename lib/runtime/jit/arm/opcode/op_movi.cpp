//
// ARM64 JIT Compiler - MOVI Opcode Implementation
//

#include "../arm64_compiler.h"

using namespace asmjit;

bool Arm64Compiler::emit_movi(int value, int outRegister) {
    /*
     * VM Interpreter Code:
     * _MOVI:
     *     regs[single_arg] = raw_arg2;
     *     branch_for(2)
     */
    
    // TODO: Implement MOVI opcode
    // Move immediate value to register: registers[outRegister] = value
    return false;
}