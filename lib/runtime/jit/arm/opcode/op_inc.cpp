//
// ARM64 JIT Compiler - INC Opcode Implementation
//

#include "../arm64_compiler.h"

using namespace asmjit;

bool Arm64Compiler::emit_inc(int outRegister) {
    /*
     * VM Interpreter Code:
     * INC:
     *     regs[single_arg]++;
     *     branch
     */
    
    // TODO: Implement INC opcode
    // registers[outRegister]++;
    return false;
}