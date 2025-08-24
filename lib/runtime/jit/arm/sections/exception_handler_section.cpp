//
// Exception Handler Section for ARM64 JIT Compiler
// Handles exception processing and propagation
//

#include "../arm64_compiler.h"
#include "../../../../core/thread_state.h"
#include "../../../multitasking/thread/sharp_thread.h"
#include <cstddef>  // For offsetof
#include "../../../multitasking/thread/thread_controller.h"
#include "../../../virtual_machine.h"

using namespace asmjit;

void Arm64Compiler::generateExceptionHandlerSection() {
    // Bind the exception handler label
    assembler.bind(catchExceptionLabel);
    setCurrentPc(tempReg3);
    
    // Check if thread->state == THREAD_KILLED
    Label skipKilledCheck = assembler.newLabel();
    assembler.ldr(tempReg1, a64::ptr(threadPtr, offsetof(sharp_thread, state))); // Load thread->state
    assembler.cmp(tempReg1, THREAD_KILLED);  // Compare with THREAD_KILLED
    assembler.b_ne(skipKilledCheck);  // Branch if not equal
    
    // Thread is killed - enable exception flag and return
    assembler.mov(tempReg1, 1);  // Set second parameter to true
    callStaticFunction(reinterpret_cast<void*>(enable_exception_flag), threadPtr, tempReg1);
    emitReturn(JIT_KILL);  // Return with kill code since thread is killed
    
    assembler.bind(skipKilledCheck);
    
    // Call catch_exception() function
    callStaticFunction(reinterpret_cast<void*>(catch_exception));
    
    // Check if catch_exception() returned true (exception was caught)
    Label exceptionNotCaught = assembler.newLabel();
    assembler.cbz(returnReg, exceptionNotCaught);  // Jump if catch_exception() returned false
    
    // Exception was caught - jump to the PC that was set by catch_exception()
    // Load task pointer
    assembler.ldr(tempReg1, a64::ptr(threadPtr, offsetof(sharp_thread, task)));
    
    // Load the PC that was set by catch_exception()
    assembler.ldr(tempReg2, a64::ptr(tempReg1, offsetof(fiber, pc)));     // Load task->pc
    assembler.ldr(tempReg1, a64::ptr(tempReg1, offsetof(fiber, rom)));     // Load base bytecode address
    assembler.sub(tempReg3, tempReg2, tempReg1);                          // PC offset from base
    assembler.lsr(tempReg3, tempReg3, 2);                                 // Divide by 4 to get instruction index
    
    // Use jump table dispatch to continue execution at the catch handler
    assembler.lsl(tempReg1, tempReg3, 3);                                 // targetPC * 8
    assembler.add(tempReg2, jumpTablePtr, tempReg1);                      // jumpTablePtr + offset
    assembler.ldr(tempReg1, a64::ptr(tempReg2));                          // Load jumpTable[targetPC]
    assembler.br(tempReg1);                                               // Jump to catch handler
    
    assembler.bind(exceptionNotCaught);
    
    // Exception was not caught - return with JIT_EXCEPTION
    emitReturn(JIT_EXCEPTION);
}