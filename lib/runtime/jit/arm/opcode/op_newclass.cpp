#include "../arm64_compiler.h"
#include "../../../virtual_machine.h"
#include "../../jit_wrappers.h"

using namespace asmjit;

bool Arm64Compiler::emit_newclass(int address) {
    /*
     * VM Interpreter Code:
     * NEWCLASS:
     *     grow_stack
     *     stack_overflow_check
     *     copy_object(&push_stack_object, create_object(&vm.classes[raw_arg2]));
     *     check_state(2)
     * 
     * Where:
     * - grow_stack = grow_stack_for(1) = if(((task->sp-task->stack)+1) >= task->stackSize) task->growStack(1);
     * - stack_overflow_check = stack_overflow_check_for(1) = if(((task->sp-task->stack)+1) >= task->stackLimit) throw vm_exception(...);
     * - push_stack_object = (++task->sp)->obj  
     * - raw_arg2 = address (class index parameter)
     * - vm.classes[address] = pointer to sharp_class structure at index
     * - create_object(&vm.classes[address]) = creates new object instance of that class
     * - copy_object(object* dest, sharp_object* src) = copies object reference to stack
     * - check_state(2) = check thread signals, advance PC by 2 instructions
     */
    
    // Step 1: grow_stack - Check/grow stack for 1 item
    Label growStackReturn = assembler->newLabel();
    emitGrowStackCheck(1, growStackReturn);
    
    // Step 2: stack_overflow_check - Check stack overflow for 1 item  
    Label stackOverflowReturn = assembler->newLabel();
    emitStackOverflowCheck(1, stackOverflowReturn);
    
    // Step 3: Create object - call create_object(&vm.classes[address])
    // Calculate vm.classes[address] = vm.classes + (address * sizeof(sharp_class))
    assembler->mov(tempReg1, reinterpret_cast<uint64_t>(vm.classes));  // Load vm.classes base address
    assembler->mov(tempReg2, address);  // Load address index
    assembler->mov(tempReg3, sizeof(sharp_class));  // Load sizeof(sharp_class)
    assembler->mul(tempReg2, tempReg2, tempReg3);  // address * sizeof(sharp_class)
    assembler->add(tempReg1, tempReg1, tempReg2);  // vm.classes + offset = &vm.classes[address]
    
    // Call jit_createObject(&vm.classes[address])
    callStaticFunction(reinterpret_cast<void*>(jit_createObject), tempReg1);
    
    // Fast exception check after object creation
    emitFastExceptionCheck(tempReg5);

    // Result is in returnReg (x0) - this is the created sharp_object*
    assembler->mov(tempReg4, returnReg);  // Save created object pointer
    
    // Step 4: push_stack_object - Get address of (++task->sp)->obj using helper
    emitPushStackObject(tempReg3);  // tempReg3 now contains &push_stack_object
    
    // Step 5: copy_object(&push_stack_object, created_object) 
    // Call jit_copyObject1(object* dest, sharp_object* src)
    callStaticFunction(reinterpret_cast<void*>(jit_copyObject1), tempReg3, tempReg4);

    // Step 6: check_state(2) - Check thread state for exceptions/signals
    emitStateCheck(2);
    
    return true;
}
