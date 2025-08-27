#include "../arm64_compiler.h"

using namespace asmjit;

bool Arm64Compiler::emit_lt(int leftRegister, int rightRegister) {
    /*
     * VM Interpreter Code:
     * LT:
     *     regs[CMT] = regs[dual_arg1] < regs[dual_arg2];
     *     branch
     */
    
    // Load values from both registers
    loadRegisterValue(tempVec1, static_cast<_register>(leftRegister));   // tempVec1 = registers[leftRegister]
    loadRegisterValue(tempVec2, static_cast<_register>(rightRegister));  // tempVec2 = registers[rightRegister]
    
    // Compare: tempVec1 < tempVec2
    assembler.fcmp(tempVec1, tempVec2);
    
    // Set result based on comparison
    // ARM64 condition: LT (less than) -> C clear and Z clear
    assembler.cset(tempReg1, a64::CondCode::kLT);  // Set tempReg1 to 1 if LT, 0 otherwise
    
    // Convert integer result (0 or 1) to floating point
    assembler.scvtf(tempVec1, tempReg1);  // Convert int to double
    
    // Store result in CMT register
    storeRegisterValue(CMT, tempVec1);
    
    return true;
}
