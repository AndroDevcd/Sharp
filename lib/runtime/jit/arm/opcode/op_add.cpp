//
// Created by Dev {Code} on 8/21/25.
//
#include "../arm64_compiler.h"

bool Arm64Compiler::emit_add(int outRegister, int leftRegister, int rightRegister) {
    // Example: registers[outRegister] = registers[leftRegister] + registers[rightRegister]
    // Using our high-level helper function
    addRegisters((_register)outRegister, (_register)leftRegister, (_register)rightRegister);
    return true;
}