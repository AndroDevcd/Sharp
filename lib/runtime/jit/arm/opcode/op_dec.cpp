//
// ARM64 JIT Compiler - DEC Opcode Implementation
//

#include "../arm64_compiler.h"

using namespace asmjit;

bool Arm64Compiler::emit_dec(int outRegister) {
    /*
     * VM Interpreter Code:
     * DEC:
     *     regs[single_arg]--;
     *     branch
     */
    
    // TODO: Implement DEC opcode
    // registers[outRegister]--;
    return false;
}