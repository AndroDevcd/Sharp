//
// State Check Helper Functions for ARM64 JIT Compiler
// Provides utilities for state checking and continue label management
//

#include "../arm64_compiler.h"

using namespace asmjit;

Label Arm64Compiler::createContinueLabel() {
    Label continueLabel = assembler.newLabel();
    continueLabels.push_back(continueLabel);
    return continueLabel;
}

void Arm64Compiler::emitStateCheck() {
    // Create a continue label for this state check
    Label continueLabel = createContinueLabel();
    
    // Store which continue label to jump back to (using stack or register)
    // We'll use tempReg3 to store the continue label index
    assembler.mov(tempReg3, continueLabels.size() - 1);
    
    // Jump to state check section
    assembler.b(stateCheckLabel);
    
    // Bind the continue label - execution resumes here after state check
    assembler.bind(continueLabel);
}