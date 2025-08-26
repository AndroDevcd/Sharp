#include "../arm64_compiler.h"

using namespace asmjit;

bool Arm64Compiler::emit_istore(int value) {
    /*
     * VM Interpreter Code:
     * ISTORE:
     *     grow_stack
     *     stack_overflow_check
     *     push_stack_number = raw_arg2;
     *     branch_for(2)
     * 
     * Where:
     * - grow_stack = grow_stack_for(1) = if(((task->sp-task->stack)+1) >= task->stackSize) task->growStack(1);
     * - stack_overflow_check = stack_overflow_check_for(1) = if(((task->sp-task->stack)+1) >= task->stackLimit) throw vm_exception(...);
     * - push_stack_number = (++task->sp)->var
     * - raw_arg2 = (int32_t)*(task->pc+1) = immediate value parameter
     * - branch_for(2) = task->pc += 2; (advance PC by 2 instructions)
     */
    
    // Step 1: grow_stack - Check if we need to grow the stack before pushing
    // This ensures the stack has enough space for the new item
    Label growStackReturn = assembler.newLabel();
    emitGrowStackCheck(1, growStackReturn);
    
    // Step 2: stack_overflow_check - Check if we would exceed the stack limit
    // This throws an exception if we're about to overflow
    Label stackOverflowReturn = assembler.newLabel();
    emitStackOverflowCheck(1, stackOverflowReturn);
    
    // Step 3: push_stack_number = raw_arg2 - Push the immediate value onto stack
    // This increments task->sp and stores the int32_t value as a double in the var field
    emitPushStackNumberImmediate(value);
    
    // Step 4: branch_for(2) - Advance PC by 2 instructions (handled by JIT infrastructure)
    // The JIT compiler will automatically handle PC advancement
    // No explicit PC manipulation needed here
    
    return true;
}
