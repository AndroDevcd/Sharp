//
// Return Section for ARM64 JIT Compiler
// Handles centralized function return and cleanup
//

#include "../arm64_compiler.h"

using namespace asmjit;

void Arm64Compiler::generateReturnSection() {
    // Generate centralized return section
    assembler.bind(returnFromFunctionLabel);
    
    // Function return epilogue - restore all saved registers in reverse order
    // Return value should already be set in returnReg before jumping here
    
    // Restore reserved stack space
    assembler.add(stackPtr, stackPtr, 16);
    
    // Restore vector callee-saved registers: d8, d9
    assembler.ldp(tempVec1, tempVec2, a64::ptr(stackPtr, 16).post());
    
    // Restore general purpose callee-saved registers: x23, x22, x21, x20, x19
    assembler.ldr(tempReg3, a64::ptr(stackPtr, 16).post());
    assembler.ldp(tempReg1, tempReg2, a64::ptr(stackPtr, 16).post());
    assembler.ldp(threadPtr, registersPtr, a64::ptr(stackPtr, 16).post());
    
    // Restore frame pointer and link register, then return
    assembler.ldp(framePtr, linkReg, a64::ptr(stackPtr, 16).post());
    assembler.ret(linkReg);
}

void Arm64Compiler::emitReturn(int returnCode) {
    // Set return code and jump to centralized return section
    assembler.mov(returnReg, returnCode);  // Use enum values: JIT_OK, JIT_CONTEXT_SWITCH, etc.
    assembler.b(returnFromFunctionLabel);
}