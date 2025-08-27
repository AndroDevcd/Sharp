#include "../arm64_compiler.h"
#include "../../../multitasking/fiber/fiber.h"
#include "../../../memory/vm_stack.h"
#include "../../../multitasking/thread/sharp_thread.h"
#include <cstddef>

using namespace asmjit;

bool Arm64Compiler::emit_loadl(int outRegister, int relFrameAddress) {
    /*
     * VM Interpreter Code:
     * LOADL:
     *     regs[dual_arg1] = (task->fp + dual_arg2)->var;
     *     branch
     */
    
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
    
    // Store the value to registers[outRegister]
    storeRegisterValue(static_cast<_register>(outRegister), tempVec1);
    
    return true;
}
