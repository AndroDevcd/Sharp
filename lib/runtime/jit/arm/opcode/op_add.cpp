//
// ARM64 JIT Compiler - ADD Opcode Implementation
//
#include "../arm64_compiler.h"

bool Arm64Compiler::emit_add(int outRegister, int leftRegister, int rightRegister) {
    /*
     * VM Interpreter Code:
     * ADD:
     *     regs[triple_arg3] = regs[triple_arg1] + regs[triple_arg2];
     *     branch
     */
    
    // registers[outRegister] = registers[leftRegister] + registers[rightRegister]
    addRegisters((_register)outRegister, (_register)leftRegister, (_register)rightRegister);
    return true;
}