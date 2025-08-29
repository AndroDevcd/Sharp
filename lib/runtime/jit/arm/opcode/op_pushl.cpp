#include "../arm64_compiler.h"
#include "../../jit_wrappers.h"

using namespace asmjit;

bool Arm64Compiler::emit_pushl(int relFrameAddress) {
    /*
     * VM Interpreter Code:
     * PUSHL:
     *     grow_stack
     *     stack_overflow_check
     *     copy_object(&push_stack_object, (task->fp + single_arg)->obj.o);
     *     branch
     * 
     * Where:
     * - single_arg = relFrameAddress (frame offset parameter)
     * - (task->fp + relFrameAddress)->obj.o = source object in local frame
     * - push_stack_object = (++task->sp)->obj (push object to stack)
     * - copy_object(object* dest, sharp_object* src) = copy object reference
     * - branch = advance PC by 1 instruction
     */
    
    // Step 1: grow_stack - Check if we need to grow the stack before pushing
    Label growStackReturn = assembler->newLabel();
    emitGrowStackCheck(1, growStackReturn);
    
    // Step 2: stack_overflow_check - Check for stack overflow
    Label stackOverflowReturn = assembler->newLabel();
    emitStackOverflowCheck(1, stackOverflowReturn);
    
    // Step 3: Get source object: (task->fp + relFrameAddress)->obj.o
    // Load task pointer from thread->task
    assembler->ldr(tempReg1, a64::ptr(threadPtr, offsetof(sharp_thread, task)));
    
    // Load frame pointer: fp = task->fp
    assembler->ldr(tempReg2, a64::ptr(tempReg1, offsetof(fiber, fp)));
    
    // Calculate frame slot address: task->fp + (relFrameAddress * sizeof(stack_item))
    assembler->mov(tempReg3, relFrameAddress);
    assembler->mov(tempReg32_1, sizeof(stack_item));
    assembler->mul(tempReg3, tempReg3, tempReg32_1);  // relFrameAddress * sizeof(stack_item)
    assembler->add(tempReg2, tempReg2, tempReg3);   // task->fp + offset
    
    // Get address of obj field: &(task->fp + relFrameAddress)->obj
    assembler->add(tempReg2, tempReg2, offsetof(stack_item, obj));  // &source->obj
    
    // Load the sharp_object* from the obj field: (task->fp + relFrameAddress)->obj.o with validation
    assembler->ldr(tempReg4, a64::ptr(tempReg2, offsetof(object, o)));  // Load source->obj.o
    
    // Add null check for object.o
    Label validSourceObject = assembler->newLabel();
    assembler->cbnz(tempReg4, validSourceObject);  // Branch if not zero (not null)
    
    // Object.o is null - set to nullptr and continue
    assembler->mov(tempReg4, 0);  // Set src parameter to nullptr
    
    assembler->bind(validSourceObject);
    
    // Step 4: push_stack_object - Get destination address for stack push using helper
    emitPushStackObject(tempReg5);  // tempReg5 now contains &push_stack_object
    
    // Step 5: copy_object(&push_stack_object, (task->fp + relFrameAddress)->obj.o)
    // Call jit_copyObject1(object* dest, sharp_object* src)
    callStaticFunction(reinterpret_cast<void*>(jit_copyObject1), tempReg5, tempReg4);
    
    // Step 6: Fast exception check after copy operation
    emitFastExceptionCheck(tempReg3);

    return true;
}
