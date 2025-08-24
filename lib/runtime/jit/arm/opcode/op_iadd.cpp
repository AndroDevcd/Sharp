//
// ARM64 JIT Compiler - IADD Opcode Implementation
//

#include "../arm64_compiler.h"

using namespace asmjit;

bool Arm64Compiler::emit_iadd(int outRegister, int value) {
    /*
     * VM Interpreter Code:
     * IADD:
     *     regs[single_arg] += raw_arg2;
     *     branch_for(2)
     */
    
    // TODO: Implement IADD opcode
    return false;
}