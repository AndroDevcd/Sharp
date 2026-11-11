//
// Exception Handler Section for x86-64 JIT Compiler
// Handles exception processing and propagation
//

#include "../x64_compiler.h"
#include "../../../../core/thread_state.h"
#include "../../../multitasking/thread/sharp_thread.h"
#include "../../../multitasking/thread/thread_controller.h"
#include "../../../virtual_machine.h"
#include "../../jit_wrappers.h"
#include <cstddef>  // For offsetof

using namespace asmjit;

/**
 * This code represents the following vm code (virtual_machine.cpp, catch_exception:)
 *
 *   catch_exception:
 *       if(thread->state == THREAD_KILLED) {
 *           enable_exception_flag(thread, true);
 *           return;
 *       }
 *
 *       if(!catch_exception()) {
 *           // nobody in this function handles it: unwind to the caller.
 *           // (the return section does return_method() + enable_exception_flag)
 *           return JIT_EXCEPTION;
 *       }
 *
 *       goto run;  // catch_exception() pointed task->pc at the handler
 *
 * pcReg must hold the pc of the instruction that raised the exception: it is
 * written to task->pc first because catch_exception() uses it to find the
 * enclosing try block.
 *
 * REGISTERS USED: everything a C call destroys
 */
void X64Compiler::generateExceptionHandlerSection() {
    assembler->bind(catchExceptionLabel);

    // task->pc = rom + pcReg
    storePC();

    // if(thread->state == THREAD_KILLED) { enable_exception_flag(thread, true); return; }
    Label notKilled = assembler->newLabel();
    assembler->cmp(x86::dword_ptr(threadPtr, offsetof(sharp_thread, state)), THREAD_KILLED);
    assembler->jne(notKilled);
    assembler->mov(arg0, threadPtr);
    assembler->mov(arg1.r32(), 1);
    callFunction(reinterpret_cast<void*>(jit_enableExceptionFlag));
    emitReturn(JIT_KILL);
    assembler->bind(notKilled);

    // if(catch_exception()) -> continue at the handler it selected
    Label exceptionNotCaught = assembler->newLabel();
    callFunction(reinterpret_cast<void*>(jit_catchException));
    assembler->test(x86::al, x86::al);            // bool result is only defined in al
    assembler->jz(exceptionNotCaught);

    // pcReg = task->pc - task->rom, then go through the state check / dispatch
    loadPC();
    assembler->jmp(stateCheckLabel);

    assembler->bind(exceptionNotCaught);
    emitReturn(JIT_EXCEPTION);
}
