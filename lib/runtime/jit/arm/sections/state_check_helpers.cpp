//
// State Check Helper Functions for ARM64 JIT Compiler
// Provides utilities for state checking and continue label management
//

#include "../arm64_compiler.h"

using namespace asmjit;

void Arm64Compiler::emitStateCheck(size_t offset) {
    // Only update PC if offset is non-zero
    // Add offset to pcReg and store the result
    assembler->add(pcReg, pcReg, offset);
    storePC();
    
    // Jump to state check section
    assembler->b(stateCheckLabel);
}

void Arm64Compiler::emitStateCheckNext() {
    // Emit state check that resumes at the next instruction
    emitStateCheck(1);
}