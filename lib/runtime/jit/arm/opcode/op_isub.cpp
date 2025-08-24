//
// ARM64 JIT Compiler - ISUB Opcode Implementation
//

#include "../arm64_compiler.h"

using namespace asmjit;

bool Arm64Compiler::emit_isub(int outRegister, int value) {
    /*
     * VM Interpreter Code:
     * ISUB:
     *     regs[single_arg] -= raw_arg2;
     *     branch_for(2)
     */
    
    // TODO: Implement ISUB opcode
    return false;
}