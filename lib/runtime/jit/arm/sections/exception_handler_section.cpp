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
#include "../../jit_wrappers.h"

using namespace asmjit;

void Arm64Compiler::generateExceptionHandlerSection() {
    // Bind the exception handler label
    assembler.bind(catchExceptionLabel);
    // Store current PC from pcReg to task->pc
    storePC();
    
    // Check if thread->state == THREAD_KILLED
    Label skipKilledCheck = assembler.newLabel();
    assembler.ldr(tempReg1, a64::ptr(threadPtr, offsetof(sharp_thread, state))); // Load thread->state
    assembler.cmp(tempReg1, THREAD_KILLED);  // Compare with THREAD_KILLED
    assembler.b_ne(skipKilledCheck);  // Branch if not equal
    
    // Thread is killed - enable exception flag and return
    assembler.mov(tempReg1, 1);  // Set second parameter to true
    callStaticFunction(reinterpret_cast<void*>(jit_enableExceptionFlag), threadPtr, tempReg1);
    emitReturn(JIT_KILL);  // Return with kill code since thread is killed
    
    assembler.bind(skipKilledCheck);
    
    // Call catch_exception() function using wrapper for crash protection
    callStaticFunction(reinterpret_cast<void*>(jit_catchException));
    
    // Check if catch_exception() returned true (exception was caught)
    Label exceptionNotCaught = assembler.newLabel();
    assembler.cbz(returnReg, exceptionNotCaught);  // Jump if catch_exception() returned false
    
    // Exception was caught - jump to the PC that was set by catch_exception()
    // Load the updated PC from task->pc into pcReg using existing helper
    loadPC();  // This loads task->pc into pcReg as instruction index
    
    // Use jump table dispatch with pcReg to continue execution at the catch handler
    assembler.lsl(tempReg1, pcReg, 3);                                    // pcReg * 8 (pointer size)
    assembler.add(tempReg2, jumpTablePtr, tempReg1);                      // jumpTablePtr + offset
    assembler.ldr(tempReg1, a64::ptr(tempReg2));                          // Load jumpTable[pcReg]
    assembler.br(tempReg1);                                               // Jump to catch handler
    
    assembler.bind(exceptionNotCaught);
    
    // Exception was not caught - return with JIT_EXCEPTION
    emitReturn(JIT_EXCEPTION);
}