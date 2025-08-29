//
// Created by BraxtonN on 2/12/2018.
//

#include "../../../../runtime/jit/arm/arm64_compiler.h"
#include "../../../virtual_machine.h"
#include "../../jit_wrappers.h"

using namespace asmjit;

void Arm64Compiler::generateIllegalBranchSection() {
    // Bind the label for the illegal branch handler
    assembler->bind(illegalBranchLabel);
    
    // This section handles cases where execution tries to branch to an illegal location
    // (e.g., jumping to an opcode argument instead of the start of an instruction)
    
    // Store the current PC (already in pcReg)
    storePC();
    
    // Load vm.invalid_operation_except class pointer into temp register
    assembler->mov(tempReg4, reinterpret_cast<uint64_t>(vm.invalid_operation_except));
    
    // Load error message address into temp register
    assembler->mov(tempReg5, reinterpret_cast<uint64_t>("Attempt to branch to invalid code section"));
    
    // Call jit_throwException(vm.invalid_operation_except, "Attempt to branch to invalid code section")
    callStaticFunction(reinterpret_cast<void*>(jit_throwException), tempReg4, tempReg5);
    
    // Call jit_enableExceptionFlag(thread, true)
    assembler->mov(tempReg1, 1);  // Set second parameter to true
    callStaticFunction(reinterpret_cast<void*>(jit_enableExceptionFlag), threadPtr, tempReg1);
    
    // Jump to catch exception section
    assembler->b(catchExceptionLabel);
}