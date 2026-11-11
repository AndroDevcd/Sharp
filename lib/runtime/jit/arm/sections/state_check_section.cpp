//
// State Check Section for ARM64 JIT Compiler
// Handles thread signal checking and dispatching
//

#include "../arm64_compiler.h"
#include "../../../../core/thread_state.h"
#include "../../../multitasking/thread/sharp_thread.h"
#include "../../../multitasking/thread/thread_controller.h"
#include "../../jit_wrappers.h"
#include <cstddef>  // For offsetof

using namespace asmjit;

/**
 * This code represent the following vm code:
 *   state_check:
 *   if(thread->signal) {
 *       if (hasSignal(thread->signal, tsig_suspend))
 *           suspend_self();
 *       if(hasSignal(thread->signal, tsig_except))
 *           goto catch_exception;
 *       if (hasSignal(thread->signal, tsig_context_switch)) {
 *           if(thread->nativeCalls == 0) {
 *               return;
 *           }
 *       }
 *       if (hasSignal(thread->signal, tsig_kill) || thread->state == THREAD_KILLED)
 *           return;
 *   }
 *
 *   DISPATCH();
 *
 * IMPORTANT: hasSignal(sig, t) is ((sig >> t) & 1U) and sendSignal sets
 * (1UL << t). The tsig_* enum VALUE is used as the bit INDEX, so
 *   tsig_except         (0x1) lives in bit 1
 *   tsig_suspend        (0x2) lives in bit 2
 *   tsig_kill           (0x4) lives in bit 4
 *   tsig_context_switch (0x8) lives in bit 8
 * The bit numbers below are therefore the enum values themselves.
 *
 * REGISTERS USED: tempReg1, tempReg2, pcReg, jumpTablePtr, jitFunctionPtr
 **/
void Arm64Compiler::generateStateCheckSection() {
    assembler->bind(stateCheckLabel);

    Label continueDispatch = assembler->newLabel();

    // if(thread->signal == 0) dispatch
    assembler->ldr(tempReg2, a64::ptr(threadPtr, offsetof(sharp_thread, signal)));
    assembler->cbz(tempReg2, continueDispatch);

    // suspend
    Label skipSuspend = assembler->newLabel();
    assembler->tbz(tempReg2, tsig_suspend, skipSuspend);
    callStaticFunction(reinterpret_cast<void*>(jit_suspendSelf));
    assembler->bind(skipSuspend);

    // The call above destroys tempReg1 and the signal word may have changed while
    // we were suspended, so always re-read it (tempReg2 is callee-saved).
    assembler->ldr(tempReg2, a64::ptr(threadPtr, offsetof(sharp_thread, signal)));

    // exception
    Label skipException = assembler->newLabel();
    assembler->tbz(tempReg2, tsig_except, skipException);
    emitExceptionHandle();
    assembler->bind(skipException);

    // context switch (only when not inside a native call)
    Label skipContextSwitch = assembler->newLabel();
    assembler->tbz(tempReg2, tsig_context_switch, skipContextSwitch);
    assembler->ldr(tempReg1, a64::ptr(threadPtr, offsetof(sharp_thread, nativeCalls)));
    assembler->cbnz(tempReg1, skipContextSwitch);
    emitReturn(JIT_CONTEXT_SWITCH);
    assembler->bind(skipContextSwitch);

    // kill signal
    Label skipKillSignal = assembler->newLabel();
    assembler->tbz(tempReg2, tsig_kill, skipKillSignal);
    emitReturn(JIT_KILL);
    assembler->bind(skipKillSignal);

    // thread->state == THREAD_KILLED (state is a 32-bit field: load it as 32-bit)
    assembler->ldr(tempReg1.w(), a64::ptr(threadPtr, offsetof(sharp_thread, state)));
    assembler->cmp(tempReg1.w(), THREAD_KILLED);
    assembler->b_ne(continueDispatch);
    emitReturn(JIT_KILL);

    // DISPATCH(): jump to jumpTable[pcReg]
    assembler->bind(continueDispatch);

    // Never index outside the table (e.g. state check after the last instruction,
    // or a pc that belongs to another function).
    assembler->ldr(tempReg1, a64::ptr(jitFunctionPtr, offsetof(jit_compiled_function, jumpTableSize)));
    assembler->cmp(pcReg, tempReg1);
    assembler->b_hs(illegalBranchLabel);

    assembler->lsl(tempReg1, pcReg, 3);                // pcReg * sizeof(void*)
    assembler->add(tempReg2, jumpTablePtr, tempReg1);  // &jumpTable[pcReg]
    assembler->ldr(tempReg1, a64::ptr(tempReg2));      // jumpTable[pcReg]
    assembler->br(tempReg1);
}
