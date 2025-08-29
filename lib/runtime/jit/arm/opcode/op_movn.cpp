#include "../arm64_compiler.h"
#include "../../../multitasking/thread/sharp_thread.h"

using namespace asmjit;

bool Arm64Compiler::emit_movn(int address) {
    /*
     * VM Interpreter Code:
     * MOVN:
     *     require_object_with_value(
     *         #ifdef SAFE_EXECUTION
     *         if(task->ptr->o->type <= type_var) {
     *             vm_exception err(vm.ill_state_except, "movn on number");
     *             enable_exception_flag(thread, true);
     *             goto catch_exception;
     *         }
     *         #endif
     *
     *         task->ptr = task->ptr->o->node + raw_arg2;
     *     )
     *     branch_for(2)
     * 
     * Where:
     * - raw_arg2 = address (second word of instruction)
     * - task->ptr->o->node = array of object pointers in the Sharp object
     * - task->ptr = task->ptr->o->node + address (navigate to specific array element)
     * - branch_for(2) = advance PC by 2 instructions (two-word instruction)
     */
    
    // Step 1: require_object_with_value - Validate task->ptr and task->ptr->o
    Label objectCheckReturn = assembler->newLabel();
    emitRequireObjectWithValue(objectCheckReturn);
    
    // Step 2: Navigate to array element: task->ptr = task->ptr->o->node + address
    // Get task pointer: task = thread->task (preserve this in tempReg1)
    assembler->ldr(tempReg1, a64::ptr(threadPtr, offsetof(sharp_thread, task)));
    
    // Load task->ptr
    assembler->ldr(tempReg3, a64::ptr(tempReg1, offsetof(fiber, ptr)));
    
    // Load task->ptr->o
    assembler->ldr(tempReg4, a64::ptr(tempReg3, offsetof(object, o)));
    
    // Load task->ptr->o->node (array of object pointers)
    assembler->ldr(tempReg5, a64::ptr(tempReg4, offsetof(sharp_object, node)));
    
    // Simple pointer arithmetic: task->ptr = task->ptr->o->node + address
    // Calculate: node + (address * sizeof(object*))
    assembler->mov(tempReg32_1, address);
    assembler->mov(tempReg6, sizeof(object*));  // sizeof(object*) = 8 bytes on 64-bit
    assembler->mul(tempReg32_1, tempReg32_1, tempReg6);  // address * sizeof(object*)
    assembler->add(tempReg5, tempReg5, tempReg32_1);  // node + (address * sizeof(object*))
    
    // Store the new pointer back to task->ptr (tempReg1 still holds task pointer)
    assembler->str(tempReg5, a64::ptr(tempReg1, offsetof(fiber, ptr)));
    
    // No additional state check needed - just normal branch (advance PC by 2 for two-word instruction)
    // Note: The PC advancement is handled by the caller in arm64_compiler.cpp
    
    return true;
}
