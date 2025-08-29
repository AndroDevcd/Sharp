#include "../arm64_compiler.h"
#include "../../../multitasking/thread/sharp_thread.h"

using namespace asmjit;

bool Arm64Compiler::emit_iload(int outRegister) {
    /*
     * VM Interpreter Code:
     * ILOAD:
     *     require_numeric_object_with_value(
     *         #ifdef SAFE_EXECUTION
     *         if(task->ptr->o->type > type_var) {
     *             vm_exception err(vm.ill_state_except, "numeric access on data structure");
     *             enable_exception_flag(thread, true);
     *             goto catch_exception;
     *         }
     *         #endif
     *
     *         regs[single_arg] = task->ptr->o->HEAD[0];
     *     )
     *     branch
     * 
     * Where:
     * - single_arg = outRegister (destination VM register)
     * - task->ptr->o->HEAD = array of numeric values
     * - task->ptr->o->HEAD[0] = first element of the array
     * - regs[outRegister] = loaded first array element value
     */
    
    // Step 1: require_numeric_object_with_value - Validate task->ptr, task->ptr->o, and type <= type_var
    Label numericCheckReturn = assembler->newLabel();
    emitRequireNumericObjectWithValue(numericCheckReturn);
    
    // Step 2: Load first array element: regs[outRegister] = task->ptr->o->HEAD[0]
    
    // Get task pointer: task = thread->task
    assembler->ldr(tempReg1, a64::ptr(threadPtr, offsetof(sharp_thread, task)));
    
    // Load task->ptr
    assembler->ldr(tempReg3, a64::ptr(tempReg1, offsetof(fiber, ptr)));
    
    // Load task->ptr->o
    assembler->ldr(tempReg4, a64::ptr(tempReg3, offsetof(object, o)));
    
    // Load task->ptr->o->HEAD (numeric array data)
    assembler->ldr(tempReg5, a64::ptr(tempReg4, offsetof(sharp_object, HEAD)));
    
    // Load the first array element (index 0) into tempVec1
    assembler->ldr(tempVec1, a64::ptr(tempReg5));  // HEAD[0] - first element
    
    // Store the loaded value into VM register: regs[outRegister] = loaded_value
    storeRegisterValue((_register)outRegister, tempVec1);
    
    // No additional state check needed - just normal branch (advance PC by 1)
    
    return true;
}
