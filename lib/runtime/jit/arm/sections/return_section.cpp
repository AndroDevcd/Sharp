//
// Return Section for ARM64 JIT Compiler
// Handles centralized function return and cleanup
//

#include "../arm64_compiler.h"
#include "../../../virtual_machine.h"
#include "../../../multitasking/fiber/fiber.h"
#include "../../../types/sharp_function.h"
#include "../../../multitasking/thread/sharp_thread.h"
#include "../../../multitasking/thread/thread_controller.h"
#include "../../jit_wrappers.h"

using namespace asmjit;

void Arm64Compiler::generateReturnSection() {
    // Generate centralized return section
    assembler->bind(returnFromFunctionLabel);
    
    // Return value is in returnReg, current PC is in tempReg3
    // Check if this is a normal return (not context switch or kill)
    Label skipVmUpdates = assembler->newLabel();
    
    // Handle context switch - skip return_method() call
    assembler->cmp(returnReg, JIT_CONTEXT_SWITCH);
    assembler->b_eq(skipVmUpdates);
    
    // Skip VM updates for kill returns
    assembler->cmp(returnReg, JIT_KILL);
    assembler->b_eq(skipVmUpdates);
    
    // This is a normal return - perform VM state management
    // Save our JIT return code before VM function calls overwrite returnReg
    assembler->mov(tempReg4, returnReg);  // Save JIT return code in tempReg4
    
    // Handle exception flag when returnCode == JIT_EXCEPTION
    // if(data == ERR_STATE) enable_exception_flag(thread, true);
    Label skipExceptionFlag = assembler->newLabel();
    assembler->cmp(tempReg4, JIT_EXCEPTION);  // Compare saved return code
    assembler->b_ne(skipExceptionFlag);
    
    // Call enable_exception_flag(thread, true) using wrapper for crash protection
    assembler->mov(tempReg1, 1);  // Set second parameter to true
    callStaticFunction(reinterpret_cast<void*>(jit_enableExceptionFlag), threadPtr, tempReg1);
    
    assembler->bind(skipExceptionFlag);

    // Call return_method()
    callStaticFunction(reinterpret_cast<void*>(return_method));
    
    // Always increment task->pc to next instruction
    // Since pc is opcode_instr* (unsigned int*), we need to add sizeof(opcode_instr) = 4 bytes
    assembler->ldr(tempReg1, a64::ptr(threadPtr, offsetof(sharp_thread, task)));  // tempReg1 = thread->task
    assembler->ldr(tempReg2, a64::ptr(tempReg1, offsetof(fiber, pc)));            // tempReg2 = task->pc
    assembler->add(tempReg2, tempReg2, sizeof(opcode_instr));                     // tempReg2 = task->pc + 4 (next instruction)
    assembler->str(tempReg2, a64::ptr(tempReg1, offsetof(fiber, pc)));            // task->pc = tempReg2
    
    // Restore our JIT return code before returning
    assembler->mov(returnReg, tempReg4);  // Restore JIT return code to returnReg

    assembler->bind(skipVmUpdates);
    // Function return epilogue - restore all saved registers in reverse order
    
    // Restore reserved stack space
    assembler->add(stackPtr, stackPtr, 16);
    
    // Restore vector callee-saved registers: d8, d9
    assembler->ldp(tempVec1, tempVec2, a64::ptr(stackPtr, 16).post());
    
    // Restore additional caller-saved registers: x11, x10, x15, x14, x13, x12 (reverse order)
    assembler->ldp(a64::x10, a64::x11, a64::ptr(stackPtr, 16).post());  // Restore x10, x11 together
    assembler->ldp(tempReg8, tempReg9, a64::ptr(stackPtr, 16).post());  // Restore x14, x15 together
    assembler->ldp(tempReg6, tempReg7, a64::ptr(stackPtr, 16).post());
    
    // Restore general purpose callee-saved registers: x27, x26, x25, x24, x23, x22, x21, x20, x19 (reverse order)
    assembler->ldp(pcReg, tempReg5, a64::ptr(stackPtr, 16).post());  // Restore x27, x28 together
    assembler->ldp(jitFunctionPtr, tempReg4, a64::ptr(stackPtr, 16).post());  // Restore x25, x26 together
    assembler->ldp(tempReg3, jumpTablePtr, a64::ptr(stackPtr, 16).post());
    assembler->ldp(tempReg1, tempReg2, a64::ptr(stackPtr, 16).post());
    assembler->ldp(threadPtr, registersPtr, a64::ptr(stackPtr, 16).post());
    
    // Restore frame pointer and link register, then return
    assembler->ldp(framePtr, linkReg, a64::ptr(stackPtr, 16).post());
    
    // Return with proper JIT status code (returnReg contains the saved JIT return code)
    // Function signature: int (*jit_function_ptr)(sharp_thread*, jit_compiled_function*, long double*)
    assembler->ret(linkReg);
}

void Arm64Compiler::emitReturn(int returnCode) {
    // Set return code and jump to centralized return section
    assembler->mov(returnReg, returnCode);  // Use enum values: JIT_KILL

    // Jump to centralized return section
    assembler->b(returnFromFunctionLabel);
}