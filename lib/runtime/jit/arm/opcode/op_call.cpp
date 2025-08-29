#include "../arm64_compiler.h"
#include "../../jit_wrappers.h"

using namespace asmjit;

bool Arm64Compiler::emit_call(int address) {
    /*
     * VM Interpreter Code:
     * CALL:
     *     prepare_method(single_arg);
     *     branch_for(0)
     * 
     * Where:
     * - single_arg = address (function address parameter)
     * - prepare_method(address) = prepares and calls the method at the given address
     * - branch_for(0) = check_state(0) = full state check with no PC advancement
     */
    
    // Store current PC before making the call
    storePC();
    
    // Call jit_prepareMethod(address)
    assembler->mov(tempReg1, address);
    callStaticFunction(reinterpret_cast<void*>(jit_prepareMethod), tempReg1);
    loadPC();
    
    // Full state check with no PC advancement (branch_for(0) = check_state(0))
    emitStateCheck(0);
    return true;
}
