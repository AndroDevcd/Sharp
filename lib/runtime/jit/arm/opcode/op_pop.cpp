#include "../arm64_compiler.h"
#include "../../jit_wrappers.h"

using namespace asmjit;

bool Arm64Compiler::emit_pop() {
    /*
     * VM Interpreter Code:
     * POP:
     *     copy_object(&pop_stack_object, (sharp_object*)nullptr);
     *     branch
     * 
     * Where:
     * - pop_stack_object = (task->sp--)->obj (pop object from stack)
     * - &pop_stack_object = address of the popped object
     * - copy_object(dest, nullptr) = clear/nullify the object
     * - branch = advance PC by 1 instruction
     */
    
    // Step 1: pop_stack_object - Get address of popped stack object using helper
    emitPopStackObject(tempReg6);  // tempReg6 now contains &pop_stack_object
    
    // Step 2: copy_object(&pop_stack_object, nullptr)
    // Call jit_copyObject1(object* dest, sharp_object* src) with src = nullptr
    assembler->mov(tempReg2, 0);  // Set nullptr (0) as second parameter
    callStaticFunction(reinterpret_cast<void*>(jit_copyObject1), tempReg6, tempReg2);
    
    // Step 3: Fast exception check after copy operation
    emitFastExceptionCheck(tempReg3);
    
    // No additional state check needed - just normal branch (advance PC by 1)
    
    return true;
}
