#include "../arm64_compiler.h"
#include "../../../multitasking/fiber/fiber.h"
#include "../../../memory/vm_stack.h"
#include "../../../multitasking/thread/sharp_thread.h"
#include <cstddef>

using namespace asmjit;

bool Arm64Compiler::emit_ipushl(int relFrameAddress) {
    /*
     * VM Interpreter Code:
     * IPUSHL:
     *     grow_stack
     *     stack_overflow_check
     *     push_stack_number = (task->fp + single_arg)->var;
     *     branch
     */
    
    // Step 1: grow_stack - Check if we need to grow the stack before pushing
    Label growStackReturn = assembler.newLabel();
    emitGrowStackCheck(1, growStackReturn);
    
    // Step 2: stack_overflow_check - Check if we would exceed the stack limit
    Label stackOverflowReturn = assembler.newLabel();
    emitStackOverflowCheck(1, stackOverflowReturn);
    
    // Load task pointer from thread->task
    assembler.ldr(tempReg1, a64::ptr(threadPtr, offsetof(sharp_thread, task)));
    
    // Load task->fp (frame pointer)
    assembler.ldr(tempReg2, a64::ptr(tempReg1, offsetof(fiber, fp)));
    
    // Calculate source address: task->fp + relFrameAddress
    // Each stack_item is sizeof(stack_item) bytes
    assembler.mov(tempReg3, relFrameAddress);
    assembler.mov(tempReg4, sizeof(stack_item));
    assembler.mul(tempReg3, tempReg3, tempReg4); // relFrameAddress * sizeof(stack_item)
    assembler.add(tempReg2, tempReg2, tempReg3); // fp + offset
    
    // Load the value from (task->fp + relFrameAddress)->var
    assembler.ldr(tempVec1, a64::ptr(tempReg2, offsetof(stack_item, var)));
    
    // Push the value to stack using helper function
    pushStackNumber(tempVec1);
    
    return true;
}
