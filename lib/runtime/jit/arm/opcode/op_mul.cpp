//
// ARM64 JIT Compiler - MUL Opcode Implementation
//

#include "../arm64_compiler.h"

using namespace asmjit;

bool Arm64Compiler::emit_mul(int outRegister, int leftRegister, int rightRegister) {
    /*
     * VM Interpreter Code:
     * MUL:
     *     regs[triple_arg3] = regs[triple_arg1] * regs[triple_arg2];
     *     branch
     */
    
    // TODO: Implement MUL opcode
    // registers[outRegister] = registers[leftRegister] * registers[rightRegister]
    return false;
}