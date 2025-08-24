//
// ARM64 JIT Compiler - DIV Opcode Implementation
//

#include "../arm64_compiler.h"

using namespace asmjit;

bool Arm64Compiler::emit_div(int outRegister, int leftRegister, int rightRegister) {
    /*
     * VM Interpreter Code:
     * DIV:
     *     regs[triple_arg3] = regs[triple_arg1] / regs[triple_arg2];
     *     branch
     */
    
    // TODO: Implement DIV opcode
    // registers[outRegister] = registers[leftRegister] / registers[rightRegister]
    return false;
}