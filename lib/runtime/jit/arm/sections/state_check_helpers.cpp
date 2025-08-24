//
// State Check Helper Functions for ARM64 JIT Compiler
// Provides utilities for state checking and continue label management
//

#include "../arm64_compiler.h"

using namespace asmjit;

void Arm64Compiler::emitStateCheck(size_t targetPC) {
    // Set the target PC in tempReg3 for return dispatch
    assembler.mov(tempReg3, targetPC);
    
    // Jump to state check section
    assembler.b(stateCheckLabel);
    
    // NOTE: Execution will resume at targetPC after state check completes
    // The state check section will use the jump table to return to jumpTable[targetPC]
}

void Arm64Compiler::emitStateCheckNext() {
    // Emit state check that resumes at the next instruction
    emitStateCheck(currentPC + 1);
}