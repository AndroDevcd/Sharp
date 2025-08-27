#include "../arm64_compiler.h"
#include "../../../multitasking/fiber/fiber.h"
#include "../../../memory/vm_stack.h"
#include "../../../multitasking/thread/sharp_thread.h"
#include <cstddef>

using namespace asmjit;

bool Arm64Compiler::emit_iaddl(int value, int relFrameAddress) {
    /*
     * VM Interpreter Code:
     * IADDL:
     *     (task->fp + single_arg)->var += raw_arg2;
     *     branch_for(2)
     * 
     * Where:
     * - single_arg = relFrameAddress (frame offset)
     * - raw_arg2 = value (immediate value to add)
     * - branch_for(2) = advance PC by 2 instructions
     */
    
    // Load task pointer from thread->task
    assembler.ldr(tempReg1, a64::ptr(threadPtr, offsetof(sharp_thread, task)));
    
    // Load task->fp (frame pointer)
    assembler.ldr(tempReg2, a64::ptr(tempReg1, offsetof(fiber, fp)));
    
    // Calculate target address: task->fp + relFrameAddress
    // Each stack_item is sizeof(stack_item) bytes
    assembler.mov(tempReg3, relFrameAddress);
    assembler.mov(tempReg4, sizeof(stack_item));
    assembler.mul(tempReg3, tempReg3, tempReg4); // relFrameAddress * sizeof(stack_item)
    assembler.add(tempReg2, tempReg2, tempReg3); // fp + offset
    
    // Load current value from (task->fp + relFrameAddress)->var
    assembler.ldr(tempVec1, a64::ptr(tempReg2, offsetof(stack_item, var)));
    
    // Convert immediate value to floating point and add
    assembler.mov(tempReg3, value);  // Load immediate value
    assembler.scvtf(tempVec2, tempReg3);  // Convert int to double
    assembler.fadd(tempVec1, tempVec1, tempVec2);  // tempVec1 += tempVec2
    
    // Store result back to (task->fp + relFrameAddress)->var
    assembler.str(tempVec1, a64::ptr(tempReg2, offsetof(stack_item, var)));
    
    return true;
}
