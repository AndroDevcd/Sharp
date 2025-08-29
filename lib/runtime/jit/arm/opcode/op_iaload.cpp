#include "../arm64_compiler.h"
#include "../../../multitasking/thread/sharp_thread.h"

using namespace asmjit;

bool Arm64Compiler::emit_iaload(int outRegister, int indexRegister) {
    /*
     * VM Interpreter Code:
     * IALOAD:
     *     require_numeric_object_with_value(
     *         #ifdef SAFE_EXECUTION
     *         if(task->ptr->o->type > type_var) {
     *             vm_exception err(vm.ill_state_except, "numeric access on data structure");
     *             enable_exception_flag(thread, true);
     *             goto catch_exception;
     *         }
     *         #endif
     *
     *         regs[dual_arg1] = task->ptr->o->HEAD[(Int)regs[dual_arg2]];
     *     )
     *     branch
     * 
     * Where:
     * - dual_arg1 = outRegister (destination VM register)
     * - dual_arg2 = indexRegister (VM register containing array index)
     * - task->ptr->o->HEAD = array of numeric values
     * - regs[indexRegister] = index into the array
     * - regs[outRegister] = loaded array element value
     */
    
    // Step 1: require_numeric_object_with_value - Validate task->ptr, task->ptr->o, and type <= type_var
    Label numericCheckReturn = assembler->newLabel();
    emitRequireNumericObjectWithValue(numericCheckReturn);
    
    // Step 2: Load array element: regs[outRegister] = task->ptr->o->HEAD[regs[indexRegister]]
    
    // Get task pointer: task = thread->task
    assembler->ldr(tempReg1, a64::ptr(threadPtr, offsetof(sharp_thread, task)));
    
    // Load task->ptr
    assembler->ldr(tempReg3, a64::ptr(tempReg1, offsetof(fiber, ptr)));
    
    // Load task->ptr->o
    assembler->ldr(tempReg4, a64::ptr(tempReg3, offsetof(object, o)));
    
    // Load task->ptr->o->HEAD (numeric array data)
    assembler->ldr(tempReg5, a64::ptr(tempReg4, offsetof(sharp_object, HEAD)));
    
    // Load index from VM register: regs[indexRegister]
    loadRegisterValue(tempVec1, (_register)indexRegister);
    
    // Convert index from double to integer
    assembler->fcvtzs(returnReg, tempVec1);  // Convert double to signed int64
    
    // Calculate array offset: index * sizeof(long double)
    assembler->mov(tempReg3, sizeof(long double));
    assembler->mul(returnReg, returnReg, tempReg3);  // index * sizeof(long double)
    
    // Calculate final address: HEAD + (index * sizeof(long double))
    assembler->add(tempReg5, tempReg5, returnReg);
    
    // Load the array element into tempVec2
    assembler->ldr(tempVec2, a64::ptr(tempReg5));
    
    // Store the loaded value into VM register: regs[outRegister] = loaded_value
    storeRegisterValue((_register)outRegister, tempVec2);
    
    // No additional state check needed - just normal branch (advance PC by 1)
    
    return true;
}
