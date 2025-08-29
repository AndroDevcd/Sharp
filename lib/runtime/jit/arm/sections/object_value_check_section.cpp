//
// ARM64 JIT Compiler - Object Value Check Section
// Centralized require_object_with_value validation
//

#include "../arm64_compiler.h"
#include "../../jit_wrappers.h"
#include "../../../virtual_machine.h"

using namespace asmjit;

/**
 * Generate centralized object value check section
 * 
 * REGISTERS USED: threadPtr, tempReg1, tempReg2, tempReg3, tempReg4
 * 
 */
void Arm64Compiler::generateObjectValueCheckSection() {
    /*
     * Centralized object value check section
     * VM Interpreter Macro:
     * #define require_object_with_value(code) 
     *     if(task->ptr==NULL || task->ptr->o == NULL) { 
     *         vm_exception err(vm.nullptr_except, ""); 
     *         enable_exception_flag(thread, true); 
     *         goto catch_exception;   
     *     } else { code }
     * 
     * Input registers:
     * - tempReg2: return address to jump back to after successful validation
     */
    
    assembler->bind(objectValueCheckLabel);
    
    // Get task pointer: task = thread->task
    assembler->ldr(tempReg1, a64::ptr(threadPtr, offsetof(sharp_thread, task)));
    
    // Load task->ptr
    assembler->ldr(tempReg3, a64::ptr(tempReg1, offsetof(fiber, ptr)));
    
    // Check if task->ptr == NULL
    Label throwNullptrException = assembler->newLabel();
    assembler->cbz(tempReg3, throwNullptrException);  // Jump to exception if NULL
    
    // task->ptr is valid - now check if task->ptr->o == NULL
    assembler->ldr(tempReg3, a64::ptr(tempReg3, offsetof(object, o)));
    
    Label validObject = assembler->newLabel();
    assembler->cbnz(tempReg3, validObject);  // Jump to valid if not NULL
    
    // Shared exception handling block (both NULL cases end up here)
    assembler->bind(throwNullptrException);
    
    // Call jit_throwException(vm.nullptr_except, "")
    assembler->mov(a64::x0, reinterpret_cast<uint64_t>(vm.nullptr_except));
    assembler->mov(a64::x1, reinterpret_cast<uint64_t>(""));  // empty message
    callStaticFunction(reinterpret_cast<void*>(jit_throwException), a64::x0, a64::x1);
    
    // Call jit_enableExceptionFlag(thread, true)
    assembler->mov(tempReg4, 1);  // Set second parameter to true
    callStaticFunction(reinterpret_cast<void*>(jit_enableExceptionFlag), threadPtr, tempReg4);
    
    // Jump to catch exception section
    emitExceptionHandle();
    
    // Both task->ptr and task->ptr->o are valid - return to caller
    assembler->bind(validObject);
    assembler->br(tempReg2);  // Jump back to return address
}