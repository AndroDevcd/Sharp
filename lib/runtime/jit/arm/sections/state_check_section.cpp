//
// State Check Section for ARM64 JIT Compiler
// Handles thread signal checking and dispatching
//

#include "../arm64_compiler.h"
#include "../../../../core/thread_state.h"
#include "../../../multitasking/thread/sharp_thread.h"
#include "../../../multitasking/thread/thread_controller.h"
#include <cstddef>  // For offsetof

using namespace asmjit;

void Arm64Compiler::generateStateCheckSection() {
    // Bind the state check label
    assembler.bind(stateCheckLabel);
    
    // Use our stored thread pointer from the prologue
    
    // Create continue dispatch label first
    Label continueDispatch = assembler.newLabel();
    
    // Check if thread->signal is non-zero
    assembler.ldr(tempReg1, a64::ptr(threadPtr, offsetof(sharp_thread, signal))); // Load thread->signal
    assembler.cbz(tempReg1, continueDispatch); // If signal == 0, jump directly to continue dispatch
    
    // Generate signal checking code
    // Check tsig_context_switch using hasSignal(sig, sigt) logic: ((sig >> sigt) & 1U)
    // Use ARM64 bit test instruction for efficiency
    // tsig_context_switch = 0x008 -> bit position 3
    Label skipContextSwitch = assembler.newLabel();
    assembler.tbz(tempReg1, 3, skipContextSwitch);  // Test bit 3, jump if zero (bit not set)
    
    // Check if nativeCalls == 0
    assembler.ldr(tempReg2, a64::ptr(threadPtr, offsetof(sharp_thread, nativeCalls))); // Load thread->nativeCalls
    Label contextSwitchReturn = assembler.newLabel();
    assembler.cbz(tempReg2, contextSwitchReturn); // If nativeCalls == 0, return from function
    
    assembler.bind(skipContextSwitch);
    
    // Check tsig_suspend = 0x002 -> bit position 1
    Label skipSuspend = assembler.newLabel();
    assembler.tbz(tempReg1, 1, skipSuspend);  // Test bit 1, jump if zero
    
    // Call suspend_self() function
    callStaticFunction(reinterpret_cast<void*>(suspend_self));
    
    assembler.bind(skipSuspend);
    
    // Check tsig_except = 0x001 -> bit position 0
    Label skipException = assembler.newLabel();
    assembler.tbz(tempReg1, 0, skipException);  // Test bit 0, jump if zero
    
    // Jump to exception handler
    assembler.b(catchExceptionLabel);
    
    assembler.bind(skipException);
    
    // Check tsig_kill = 0x004 -> bit position 2
    Label skipKillSignal = assembler.newLabel();
    assembler.tbz(tempReg1, 2, skipKillSignal);  // Test bit 2, jump if zero
    
    // Kill signal detected - return with kill code
    emitReturn(JIT_KILL);
    
    assembler.bind(skipKillSignal);
    
    // Also check if thread->state == THREAD_KILLED (0x005)
    Label skipKillState = assembler.newLabel();
    assembler.ldr(tempReg2, a64::ptr(threadPtr, offsetof(sharp_thread, state))); // Load thread->state
    assembler.cmp(tempReg2, THREAD_KILLED);  // Compare with THREAD_KILLED (0x005)
    assembler.b_ne(skipKillState);  // Branch if not equal
    
    // Thread state is THREAD_KILLED - return with kill code
    emitReturn(JIT_KILL);
    
    assembler.bind(skipKillState);
    
    // Context switch return - return with context switch code  
    assembler.bind(contextSwitchReturn);
    emitReturn(JIT_CONTEXT_SWITCH);
    
    // Continue dispatch - jump back to appropriate continue label
    assembler.bind(continueDispatch);
    
    // Simple dispatch based on tempReg3 value
    for (size_t i = 0; i < continueLabels.size(); ++i) {
        assembler.cmp(tempReg3, i);
        assembler.b_eq(continueLabels[i]);
    }
    
    // Default: continue to first label if something goes wrong
    if (!continueLabels.empty()) {
        assembler.b(continueLabels[0]);
    }
}