//
// ARM64 JIT Compiler Helper Functions
// High-level register operations and utility functions
//

#include "arm64_compiler.h"

using namespace asmjit;

// High-level helper functions for VM register operations

void Arm64Compiler::loadRegisterValue(a64::Vec dest, _register vmReg) {
    // Load registers[vmReg] into dest (floating point register)
    // Each register is 16 bytes (long double), so offset = vmReg * 16
    assembler.ldr(dest, a64::ptr(registersPtr, vmReg * 16));
}

void Arm64Compiler::storeRegisterValue(_register vmReg, a64::Vec src) {
    // Store src into registers[vmReg] (floating point register)
    assembler.str(src, a64::ptr(registersPtr, vmReg * 16));
}

void Arm64Compiler::addRegisters(_register destReg, _register leftReg, _register rightReg) {
    // registers[destReg] = registers[leftReg] + registers[rightReg]
    loadRegisterValue(tempVec1, leftReg);   // tempVec1 = registers[leftReg]
    loadRegisterValue(tempVec2, rightReg);  // tempVec2 = registers[rightReg]
    assembler.fadd(tempVec1, tempVec1, tempVec2);  // tempVec1 = tempVec1 + tempVec2
    storeRegisterValue(destReg, tempVec1);  // registers[destReg] = tempVec1
}

void Arm64Compiler::subRegisters(_register destReg, _register leftReg, _register rightReg) {
    // registers[destReg] = registers[leftReg] - registers[rightReg]
    loadRegisterValue(tempVec1, leftReg);
    loadRegisterValue(tempVec2, rightReg);
    assembler.fsub(tempVec1, tempVec1, tempVec2);
    storeRegisterValue(destReg, tempVec1);
}

void Arm64Compiler::mulRegisters(_register destReg, _register leftReg, _register rightReg) {
    // registers[destReg] = registers[leftReg] * registers[rightReg]
    loadRegisterValue(tempVec1, leftReg);
    loadRegisterValue(tempVec2, rightReg);
    assembler.fmul(tempVec1, tempVec1, tempVec2);
    storeRegisterValue(destReg, tempVec1);
}

void Arm64Compiler::divRegisters(_register destReg, _register leftReg, _register rightReg) {
    // registers[destReg] = registers[leftReg] / registers[rightReg]
    loadRegisterValue(tempVec1, leftReg);
    loadRegisterValue(tempVec2, rightReg);
    assembler.fdiv(tempVec1, tempVec1, tempVec2);
    storeRegisterValue(destReg, tempVec1);
}

void Arm64Compiler::moveRegister(_register destReg, _register srcReg) {
    // registers[destReg] = registers[srcReg]
    loadRegisterValue(tempVec1, srcReg);
    storeRegisterValue(destReg, tempVec1);
}

void Arm64Compiler::setRegisterImmediate(_register vmReg, int64_t value) {
    // registers[vmReg] = immediate value
    assembler.mov(tempReg1, value);
    assembler.scvtf(tempVec1, tempReg1);  // Convert int to double
    storeRegisterValue(vmReg, tempVec1);
}

// External function call helpers

void Arm64Compiler::callStaticFunction(void* functionPtr) {
    // Call external C function with no parameters
    // Load function address into temp register and call
    assembler.mov(tempReg1, reinterpret_cast<uint64_t>(functionPtr));
    assembler.blr(tempReg1);
}