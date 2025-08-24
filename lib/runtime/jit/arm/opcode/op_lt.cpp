//
// ARM64 JIT Compiler - LT Opcode Implementation
//

#include "../arm64_compiler.h"

using namespace asmjit;

bool Arm64Compiler::emit_lt(int leftRegister, int rightRegister) {
    /*
     * VM Interpreter Code:
     * LT:
     *     regs[CMT] = regs[dual_arg1] < regs[dual_arg2];
     *     branch
     */
    
    // TODO: Implement LT opcode
    // registers[CMT] = registers[leftRegister] < registers[rightRegister];
    return false;
}