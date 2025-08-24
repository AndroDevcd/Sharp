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

using namespace asmjit;

void Arm64Compiler::generateReturnSection() {
    // Generate centralized return section
    assembler.bind(returnFromFunctionLabel);
    
    // Return value is in returnReg, current PC is in tempReg3
    // Check if this is a normal return (not context switch or kill)
    Label skipVmUpdates = assembler.newLabel();
    
    // Handle context switch - save PC but skip return_method() call
    Label setPc = assembler.newLabel();
    assembler.cmp(returnReg, JIT_CONTEXT_SWITCH);
    assembler.b_eq(setPc);
    
    // Skip VM updates for kill returns
    assembler.cmp(returnReg, JIT_KILL);
    assembler.b_eq(skipVmUpdates);
    
    // This is a normal return - perform VM state management
    
    // Handle exception flag when returnCode == JIT_EXCEPTION
    // if(data == ERR_STATE) enable_exception_flag(thread, true);
    Label skipExceptionFlag = assembler.newLabel();
    assembler.cmp(returnReg, JIT_EXCEPTION);
    assembler.b_ne(skipExceptionFlag);
    
    // Call enable_exception_flag(thread, true)
    assembler.mov(tempReg1, 1);  // Set second parameter to true
    callStaticFunction(reinterpret_cast<void*>(enable_exception_flag), threadPtr, tempReg1);
    
    assembler.bind(skipExceptionFlag);

    // Call return_method()
    callStaticFunction(reinterpret_cast<void*>(return_method));
    
    // Check if return_method() returned true
    Label skipPcUpdate = assembler.newLabel();
    assembler.cbz(returnReg, skipPcUpdate);  // Skip if return_method() returned false
    
    // Context switch jumps here to set PC but skip return_method()
    assembler.bind(setPc);
    
    // Check if tempReg3 contains valid PC (-1 means invalid, skip PC update)
    assembler.cmp(tempReg3, -1);
    assembler.b_eq(skipPcUpdate);  // Jump to skipPcUpdate if PC is -1
    
    // Use helper function to set current PC
    setCurrentPc(tempReg3);
    
    assembler.bind(skipPcUpdate);
    assembler.bind(skipVmUpdates);
    
    // Function return epilogue - restore all saved registers in reverse order
    
    // Restore reserved stack space
    assembler.add(stackPtr, stackPtr, 16);
    
    // Restore vector callee-saved registers: d8, d9
    assembler.ldp(tempVec1, tempVec2, a64::ptr(stackPtr, 16).post());
    
    // Restore general purpose callee-saved registers: x25, x24, x23, x22, x21, x20, x19
    assembler.ldr(jitFunctionPtr, a64::ptr(stackPtr, 16).post());     // Single register restore
    assembler.ldp(tempReg3, jumpTablePtr, a64::ptr(stackPtr, 16).post());
    assembler.ldp(tempReg1, tempReg2, a64::ptr(stackPtr, 16).post());
    assembler.ldp(threadPtr, registersPtr, a64::ptr(stackPtr, 16).post());
    
    // Restore frame pointer and link register, then return
    assembler.ldp(framePtr, linkReg, a64::ptr(stackPtr, 16).post());
    assembler.ret(linkReg);
}

void Arm64Compiler::emitReturn(int returnCode) {
    // Only allow JIT_KILL for this version (no valid PC available)
    if (returnCode == JIT_CONTEXT_SWITCH) {
        printf("ERROR: Illegal use of emitReturn(int) with returnCode=%d. Use emitReturn(int, register) for non-context switch returns.\n", returnCode);
        return;
    }
    
    // Set return code and jump to centralized return section
    assembler.mov(returnReg, returnCode);  // Use enum values: JIT_KILL
    
    // invalidate current PC in tempReg3 for return section
    assembler.mov(tempReg3, -1);
    
    // Jump to centralized return section
    assembler.b(returnFromFunctionLabel);
}

void Arm64Compiler::emitReturn(int returnCode, a64::Gp &targetPCReg) {
    // Set return code and jump to centralized return section with dynamic PC
    assembler.mov(returnReg, returnCode);  // Use enum values: JIT_OK, JIT_CONTEXT_SWITCH, etc.
    
    // Verify targetPCReg isn't tempReg3, then assign it to tempReg3
    if (targetPCReg.id() != tempReg3.id()) {
        // Move target PC from the provided register to tempReg3
        assembler.mov(tempReg3, targetPCReg);
    }
    
    // Jump to centralized return section
    assembler.b(returnFromFunctionLabel);
}