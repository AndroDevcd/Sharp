#include "../arm64_compiler.h"
#include "../../jit_wrappers.h"

using namespace asmjit;

bool Arm64Compiler::emit_popl(int relFrameAddress) {
    /*
     * VM Interpreter Code:
     * POPL:
     *     copy_object(
     *         &(task->fp+single_arg)->obj,
     *         pop_stack_object.o
     *     );
     *     branch
     * 
     * Where:
     * - single_arg = relFrameAddress (frame offset parameter)
     * - pop_stack_object = (task->sp--)->obj (pop object from stack)
     * - pop_stack_object.o = get the sharp_object* from the popped object
     * - &(task->fp + relFrameAddress)->obj = destination in local frame
     * - copy_object(object* dest, sharp_object* src) = copy object reference
     * - branch = advance PC by 1 instruction
     */
    
    // Step 1: pop_stack_object - Get address of popped stack object using helper
    emitPopStackObject(tempReg3);  // tempReg3 now contains &pop_stack_object
    
    // Step 2: Load pop_stack_object.o (the sharp_object* from the object) with validation
    // First check if object.o is not NULL
    assembler->ldr(tempReg4, a64::ptr(tempReg3, offsetof(object, o)));  // Load pop_stack_object.o
    
    // Add null check for object.o
    Label validObject = assembler->newLabel();
    assembler->cbnz(tempReg4, validObject);  // Branch if not zero (not null)
    
    // Object.o is null - set to nullptr and continue
    assembler->mov(tempReg4, 0);  // Set src parameter to nullptr
    
    assembler->bind(validObject);
    
    // Step 3: Calculate destination address: &(task->fp + relFrameAddress)->obj
    // Load task pointer from thread->task
    assembler->ldr(tempReg1, a64::ptr(threadPtr, offsetof(sharp_thread, task)));
    
    // Load frame pointer: fp = task->fp
    assembler->ldr(tempReg2, a64::ptr(tempReg1, offsetof(fiber, fp)));
    
    // Calculate frame slot address: task->fp + (relFrameAddress * sizeof(stack_item))
    assembler->mov(tempReg5, relFrameAddress);
    assembler->mov(tempReg32_1, sizeof(stack_item));
    assembler->mul(tempReg5, tempReg5, tempReg32_1);  // relFrameAddress * sizeof(stack_item)
    assembler->add(tempReg2, tempReg2, tempReg5);   // task->fp + offset
    
    // Get address of obj field: &(task->fp + relFrameAddress)->obj
    assembler->add(tempReg1, tempReg2, offsetof(stack_item, obj));  // &destination->obj
    
    // Step 4: copy_object(&(task->fp+relFrameAddress)->obj, pop_stack_object.o)
    // Call jit_copyObject1(object* dest, sharp_object* src)
    callStaticFunction(reinterpret_cast<void*>(jit_copyObject1), tempReg1, tempReg4);
    
    // Step 5: Fast exception check after copy operation
    emitFastExceptionCheck(tempReg5);
    
    // No additional state check needed - just normal branch (advance PC by 1)
    
    return true;
}
