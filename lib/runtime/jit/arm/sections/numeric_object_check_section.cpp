//
// ARM64 JIT Compiler - Numeric Object Check Section
// Centralized require_numeric_object_with_value validation
//

#include "../arm64_compiler.h"
#include "../../jit_wrappers.h"
#include "../../../virtual_machine.h"
#include "../../../memory/sharp_object.h"

using namespace asmjit;

/**
 * Generate centralized numeric object check section
 * 
 * REGISTERS USED: threadPtr, tempReg1, tempReg2, tempReg3, tempReg4
 * 
 */
void Arm64Compiler::generateNumericObjectCheckSection() {
    /*
     * Centralized numeric object check section
     * VM Interpreter Macro:
     * #define require_numeric_object_with_value(code) 
     *     if(task->ptr==NULL || task->ptr->o == NULL || task->ptr->o->type > type_var) { 
     *         vm_exception err(vm.nullptr_except, ""); 
     *         enable_exception_flag(thread, true); 
     *         goto catch_exception;   
     *     } else { code }
     * 
     * Input registers:
     * - tempReg2: return address to jump back to after successful validation
     */
    
    assembler->bind(numericObjectCheckLabel);
    
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
    assembler->cbz(tempReg3, throwNullptrException);  // Jump to exception if NULL
    
    // Both task->ptr and task->ptr->o are valid - now check type
    // Call jit_getObjectType(task->ptr->o) to handle bit-field access
    callStaticFunction(reinterpret_cast<void*>(jit_getObjectType), tempReg3);
    
    // Check if type > type_var (result is in returnReg/x0)
    assembler->cmp(returnReg, type_var);  // Compare with type_var enum constant
    assembler->b_gt(throwNullptrException);  // Jump to exception if type > type_var
    
    // All checks passed - return to caller
    assembler->b(validObject);
    
    // Shared exception handling block
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
    
    // Object is valid and numeric - return to caller
    assembler->bind(validObject);
    assembler->br(tempReg2);  // Jump back to return address
}