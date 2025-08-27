#include "../arm64_compiler.h"
#include "../../../multitasking/fiber/fiber.h"
#include "../../../memory/vm_stack.h"
#include "../../../multitasking/thread/sharp_thread.h"
#include <cstddef>

using namespace asmjit;

bool Arm64Compiler::emit_ipopl(int relFrameAddress) {
    /*
     * VM Interpreter Code:
     * IPOPL:
     *     (task->fp+single_arg)->var = pop_stack_number;
     *     branch
     */
    
    // Pop value from stack using helper function
    popStackNumber(tempVec1);
    
    // Load task pointer and frame pointer (reload task after helper function)
    assembler.ldr(tempReg1, a64::ptr(threadPtr, offsetof(sharp_thread, task)));
    assembler.ldr(tempReg3, a64::ptr(tempReg1, offsetof(fiber, fp)));
    
    // Calculate target address: task->fp + relFrameAddress
    // Each stack_item is sizeof(stack_item) bytes
    assembler.mov(tempReg4, relFrameAddress);
    assembler.mov(tempReg1, sizeof(stack_item));
    assembler.mul(tempReg4, tempReg4, tempReg1); // relFrameAddress * sizeof(stack_item)
    assembler.add(tempReg3, tempReg3, tempReg4); // fp + offset
    
    // Store the popped value to (task->fp + relFrameAddress)->var
    assembler.str(tempVec1, a64::ptr(tempReg3, offsetof(stack_item, var)));
    
    return true;
}
