#include "../arm64_compiler.h"
#include "../../jit_wrappers.h"

using namespace asmjit;

bool Arm64Compiler::emit_dup() {
    /*
     * VM Interpreter Code:
     * DUP:
     *     data = (Int)&task->sp->obj;
     *     copy_object(&push_stack_object, (object*)data);
     *     branch
     * 
     * This duplicates the top stack object by:
     * 1. Getting address of current stack top object: &task->sp->obj
     * 2. Casting it to object* (since data is Int but represents a pointer)
     * 3. Copying it to a new stack slot using push_stack_object
     * 4. No state check needed - just advance PC by 1
     */
    
    // Step 1: Get address of current stack top object: &task->sp->obj
    // Load task pointer from thread->task
    assembler->ldr(tempReg1, a64::ptr(threadPtr, offsetof(sharp_thread, task)));
    
    // Load current stack pointer: sp = task->sp
    assembler->ldr(tempReg2, a64::ptr(tempReg1, offsetof(fiber, sp)));
    
    // Calculate address of current stack top's obj field: &task->sp->obj
    assembler->add(tempReg3, tempReg2, offsetof(stack_item, obj));  // &(task->sp)->obj
    
    // Step 2: push_stack_object - Get address for destination using helper
    emitPushStackObject(tempReg4);  // tempReg4 now contains &push_stack_object
    
    // Step 3: copy_object(&push_stack_object, (object*)&task->sp->obj) 
    // Call jit_copyObject2(object* dest, object* src)
    callStaticFunction(reinterpret_cast<void*>(jit_copyObject2), tempReg4, tempReg3);
    return true;
}
