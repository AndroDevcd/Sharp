#include "../arm64_compiler.h"
#include "../../../multitasking/thread/sharp_thread.h"

using namespace asmjit;

bool Arm64Compiler::emit_popobj_2() {
    /*
     * VM Interpreter Code:
     * POPOBJ_2:
     *     task->ptr = &pop_stack_object;
     *     branch
     * 
     * Where:
     * - pop_stack_object = (task->sp--)->obj (pop object from stack)
     * - &pop_stack_object = address of the popped object
     * - task->ptr = fiber->ptr field (object* pointer in task structure)
     * - branch = advance PC by 1 instruction
     */
    
    // Step 1: pop_stack_object - Get address of popped stack object using helper
    emitPopStackObject(tempReg6);  // tempReg6 now contains &pop_stack_object
    
    // Step 2: Store the address in task->ptr
    // Load task pointer from thread->task
    assembler->ldr(tempReg2, a64::ptr(threadPtr, offsetof(sharp_thread, task)));
    
    // Store the popped object address in task->ptr
    assembler->str(tempReg6, a64::ptr(tempReg2, offsetof(fiber, ptr)));
    
    // No state check needed - just normal branch (advance PC by 1)
    
    return true;
}
