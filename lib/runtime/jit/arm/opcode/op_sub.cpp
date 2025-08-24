//
// ARM64 JIT Compiler - SUB Opcode Implementation
//

#include "../arm64_compiler.h"

using namespace asmjit;

bool Arm64Compiler::emit_sub(int outRegister, int leftRegister, int rightRegister) {
    /*
     * VM Interpreter Code:
     * SUB:
     *     regs[triple_arg3] = regs[triple_arg1] - regs[triple_arg2];
     *     branch
     */
    
    // TODO: Implement SUB opcode
    // registers[outRegister] = registers[leftRegister] - registers[rightRegister]
    return false;
}