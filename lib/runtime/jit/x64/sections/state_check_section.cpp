//
// State Check Section for x86-64 JIT Compiler
// Handles thread signal checking and dispatching to the instruction in pcReg
//

#include "../x64_compiler.h"
#include "../../../../core/thread_state.h"
#include "../../../multitasking/thread/sharp_thread.h"
#include "../../../multitasking/thread/thread_controller.h"
#include "../../jit_wrappers.h"
#include <cstddef>  // For offsetof

using namespace asmjit;

/**
 * This code represents the following vm code (virtual_machine.cpp, state_check:)
 *
 *   state_check:
 *   if(thread->signal) {
 *       if (hasSignal(thread->signal, tsig_gc))
 *           tgc_safepoint();
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
 *   DISPATCH();   // continue at the instruction pcReg points to
 *
 * IMPORTANT: hasSignal(sig, t) is ((sig >> t) & 1U) and sendSignal sets
 * (1UL << t). The tsig_* enum VALUE is used as the bit INDEX, so
 *   tsig_except         (0x1) lives in bit 1  -> mask 0x002
 *   tsig_suspend        (0x2) lives in bit 2  -> mask 0x004
 *   tsig_kill           (0x4) lives in bit 4  -> mask 0x010
 *   tsig_context_switch (0x8) lives in bit 8  -> mask 0x100
 *   tsig_gc             (0x10) lives in bit 16 -> mask 0x10000
 * which is why the masks below are written as (1 << tsig_xxx).
 *
 * The signal word is re-read from memory for every test: other threads change
 * it, and suspend_self() is a C call that destroys our temporaries anyway.
 *
 * REGISTERS USED: tmp0 (+ everything a C call destroys when suspending)
 */
void X64Compiler::generateStateCheckSection() {
    assembler->bind(stateCheckLabel);

    Label continueDispatch = assembler->newLabel();
    x86::Mem signal = x86::qword_ptr(threadPtr, offsetof(sharp_thread, signal));

    // if(thread->signal == 0) DISPATCH();
    assembler->cmp(signal, 0);
    assembler->je(continueDispatch);

    // if(hasSignal(thread->signal, tsig_gc)) tgc_safepoint();
    // Everything compiled code knows about objects is in the fiber's stack and
    // fields (nothing is kept in machine registers across this point), which is
    // what makes this a safe point for the garbage collector.
    Label skipGc = assembler->newLabel();
    assembler->test(signal, 1 << tsig_gc);
    assembler->jz(skipGc);
    callFunction(reinterpret_cast<void*>(jit_gcSafepoint));
    assembler->bind(skipGc);

    // if(hasSignal(thread->signal, tsig_suspend)) suspend_self();
    Label skipSuspend = assembler->newLabel();
    assembler->test(signal, 1 << tsig_suspend);
    assembler->jz(skipSuspend);
    callFunction(reinterpret_cast<void*>(jit_suspendSelf));
    assembler->bind(skipSuspend);

    // if(hasSignal(thread->signal, tsig_except)) goto catch_exception;
    assembler->test(signal, 1 << tsig_except);
    assembler->jnz(catchExceptionLabel);

    // if(hasSignal(thread->signal, tsig_context_switch) && thread->nativeCalls == 0) return;
    Label skipContextSwitch = assembler->newLabel();
    assembler->test(signal, 1 << tsig_context_switch);
    assembler->jz(skipContextSwitch);
    assembler->cmp(x86::qword_ptr(threadPtr, offsetof(sharp_thread, nativeCalls)), 0);
    assembler->jne(skipContextSwitch);
    emitReturn(JIT_CONTEXT_SWITCH);
    assembler->bind(skipContextSwitch);

    // if(hasSignal(thread->signal, tsig_kill) || thread->state == THREAD_KILLED) return;
    Label killed = assembler->newLabel();
    assembler->test(signal, 1 << tsig_kill);
    assembler->jnz(killed);
    assembler->cmp(x86::dword_ptr(threadPtr, offsetof(sharp_thread, state)), THREAD_KILLED); // state is 32-bit
    assembler->jne(continueDispatch);
    assembler->bind(killed);
    emitReturn(JIT_KILL);

    // DISPATCH():  goto *fun->jumpTable[pcReg];
    assembler->bind(continueDispatch);

    // Never index outside the table (state check after the last instruction, or a
    // pc that belongs to another function): if(pcReg >= fun->jumpTableSize) illegal
    assembler->cmp(pcReg, x86::qword_ptr(jitFunctionPtr, offsetof(jit_compiled_function, jumpTableSize)));
    assembler->jae(illegalBranchLabel);

    assembler->jmp(x86::qword_ptr(jumpTablePtr, pcReg, 3));   // jumpTable[pcReg], 8 bytes per entry
}
