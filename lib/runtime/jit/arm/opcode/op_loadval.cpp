#include "../arm64_compiler.h"

using namespace asmjit;

bool Arm64Compiler::emit_loadval(int outRegister) {
    /*
     * VM Interpreter Code:
     * LOADVAL:
     *     regs[single_arg] = pop_stack_number;
     *     branch
     */
    
    // Pop value from stack using helper function
    popStackNumber(tempVec1);
    
    // Store the popped value to registers[outRegister]
    storeRegisterValue(static_cast<_register>(outRegister), tempVec1);
    
    return true;
}
