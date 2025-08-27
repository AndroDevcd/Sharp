#include "../arm64_compiler.h"
#include "../../../multitasking/fiber/fiber.h"
#include "../../../memory/vm_stack.h"
#include "../../../memory/sharp_object.h"
#include "../../../multitasking/thread/sharp_thread.h"
#include <cstddef>

using namespace asmjit;

bool Arm64Compiler::emit_ret(int errState) {
    /*
     * VM Interpreter Code:
     * RET:
     *     data = single_arg;
     *     if(data == ERR_STATE && task->exceptionObject.o == nullptr) {
     *         copy_object(
     *             &task->exceptionObject,
     *             pop_stack_object.o
     *         );
     *     }
     *
     *     if(return_method()) {
     *         if(data == ERR_STATE) {
     *             enable_exception_flag(thread, true);
     *         }
     *         task->pc++;
     *         return;
     *     }
     * 
     * Where:
     * - data = errState (ERR_STATE = 1, NO_ERR = 0)
     * - ERR_STATE = 1, NO_ERR = 0
     */
    
    // Check if this is an error return (errState == ERR_STATE)
    Label skipExceptionHandling = assembler.newLabel();
    Label continueReturn = assembler.newLabel();
    
    if (errState != 1) { // ERR_STATE = 1
        // Not an error state, skip exception handling
        assembler.b(continueReturn);
    } else {
        // This is ERR_STATE - check if we need to copy exception object
        // Load task pointer
        assembler.ldr(tempReg1, a64::ptr(threadPtr, offsetof(sharp_thread, task)));
        
        // Check if task->exceptionObject.o == nullptr
        assembler.ldr(tempReg2, a64::ptr(tempReg1, offsetof(fiber, exceptionObject) + offsetof(object, o)));
        assembler.cbnz(tempReg2, skipExceptionHandling); // Skip if not null
        
        // Exception object is null, need to pop stack object and copy it
        // Prepare arguments for copy_object(&task->exceptionObject, pop_stack_object.o)
        
        // Calculate destination address: &task->exceptionObject
        assembler.add(tempReg2, tempReg1, offsetof(fiber, exceptionObject)); // tempReg2 = &task->exceptionObject
        
        // Load task->sp (stack pointer) 
        assembler.ldr(tempReg3, a64::ptr(tempReg1, offsetof(fiber, sp))); // tempReg3 = task->sp
        
        // Get source object: (task->sp)->obj.o
        assembler.ldr(tempReg4, a64::ptr(tempReg3, offsetof(stack_item, obj) + offsetof(object, o))); // tempReg4 = pop_stack_object.o
        
        // Pop the stack (decrement sp)
        assembler.sub(tempReg3, tempReg3, sizeof(stack_item));
        assembler.str(tempReg3, a64::ptr(tempReg1, offsetof(fiber, sp))); // Store decremented sp back
        
        // Call copy_object(&task->exceptionObject, pop_stack_object.o)
        // copy_object(object* dest, sharp_object* src)
        assembler.mov(a64::x0, tempReg2);  // First parameter: &task->exceptionObject
        assembler.mov(a64::x1, tempReg4);  // Second parameter: pop_stack_object.o
        
        // Cast to the correct overload: void copy_object(object *to, sharp_object *from)
        typedef void (*copy_object_ptr)(object*, sharp_object*);
        copy_object_ptr copy_func = static_cast<copy_object_ptr>(copy_object);
        assembler.mov(tempReg3, reinterpret_cast<uint64_t>(copy_func));
        assembler.blr(tempReg3);  // Call copy_object
        
        assembler.bind(skipExceptionHandling);
    }
    
    assembler.bind(continueReturn);
    
    // Determine JIT return code based on errState
    int jitReturnCode = (errState == 1) ? JIT_EXCEPTION : JIT_OK;
    
    // Return with appropriate code - the return section handles the rest
    emitReturn(jitReturnCode);
    
    return true;
}
