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
 *       if (hasSignal(thread->signal, tsig_context_switch)) {
 *           if(thread->nativeCalls == 0) {
 *               return;
 *           }
 *       }
 *       if (hasSignal(thread->signal, tsig_suspend))
 *           suspend_self();
 *       if(hasSignal(thread->signal, tsig_except))
 *           goto catch_exception;
 *       if (hasSignal(thread->signal, tsig_kill) || thread->state == THREAD_KILLED)
 *           return;
 *   }
 *
 *   DISPATCH();
 **/
void Arm64Compiler::generateStateCheckSection() {
    // Bind the state check label
    assembler->bind(stateCheckLabel);
    
    // Use our stored thread pointer from the prologue
    
    // Create continue dispatch label first
    Label continueDispatch = assembler->newLabel();
    
    // Check if thread->signal is non-zero
    assembler->ldr(tempReg1, a64::ptr(threadPtr, offsetof(sharp_thread, signal))); // Load thread->signal
    assembler->cbz(tempReg1, continueDispatch); // If signal == 0, jump directly to continue dispatch
    
    // Generate signal checking code
    // Check tsig_context_switch using hasSignal(sig, sigt) logic: ((sig >> sigt) & 1U)
    // Use ARM64 bit test instruction for efficiency
    // tsig_context_switch = 0x008 -> bit position 3
    Label skipContextSwitch = assembler->newLabel();
    assembler->tbz(tempReg1, 3, skipContextSwitch);  // Test bit 3, jump if zero (bit not set)
    
    // Check if nativeCalls == 0
    assembler->ldr(tempReg2, a64::ptr(threadPtr, offsetof(sharp_thread, nativeCalls))); // Load thread->nativeCalls
    assembler->cbnz(tempReg2, skipContextSwitch); // If nativeCalls != 0, skip context switch return
    
    // nativeCalls == 0, return with context switch code
    emitReturn(JIT_CONTEXT_SWITCH);
    
    assembler->bind(skipContextSwitch);
    
    // Check tsig_suspend = 0x002 -> bit position 1
    Label skipSuspend = assembler->newLabel();
    assembler->tbz(tempReg1, 1, skipSuspend);  // Test bit 1, jump if zero
    
    // Call suspend_self() function using wrapper for crash protection
    callStaticFunction(reinterpret_cast<void*>(jit_suspendSelf));
    
    assembler->bind(skipSuspend);
    
    // Check tsig_except = 0x001 -> bit position 0
    Label skipException = assembler->newLabel();
    assembler->tbz(tempReg1, 0, skipException);  // Test bit 0, jump if zero
    
    // Jump to exception handler
    emitExceptionHandle();
    
    assembler->bind(skipException);
    
    // Check tsig_kill = 0x004 -> bit position 2
    Label skipKillSignal = assembler->newLabel();
    assembler->tbz(tempReg1, 2, skipKillSignal);  // Test bit 2, jump if zero
    
    // Kill signal detected - return with kill code
    emitReturn(JIT_KILL);
    
    assembler->bind(skipKillSignal);
    
    // Also check if thread->state == THREAD_KILLED (0x005)
    Label skipKillState = assembler->newLabel();
    assembler->ldr(tempReg2, a64::ptr(threadPtr, offsetof(sharp_thread, state))); // Load thread->state
    assembler->cmp(tempReg2, THREAD_KILLED);  // Compare with THREAD_KILLED (0x005)
    assembler->b_ne(skipKillState);  // Branch if not equal
    
    // Thread state is THREAD_KILLED - return with kill code
    emitReturn(JIT_KILL);
    
    assembler->bind(skipKillState);
    
    // Continue dispatch - jump back to appropriate PC using jump table
    assembler->bind(continueDispatch);
    
    // pcReg contains the target PC 
    // Use jump table dispatch for O(1) PC lookup
    
    // Calculate offset: targetPC * 8 (each pointer is 8 bytes)
    assembler->lsl(tempReg1, pcReg, 3);  // pcReg << 3
    
    // Load jump table address and add offset  
    assembler->add(tempReg2, jumpTablePtr, tempReg1);  // jumpTablePtr + (pcReg * 8)
    
    // Load target address from jump table
    assembler->ldr(tempReg1, a64::ptr(tempReg2));      // Load jumpTable[pcReg]
    
    // Jump to target address
    assembler->br(tempReg1);                           // Jump to the target opcode
}