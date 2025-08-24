//
// ARM64 JIT Compiler Helper Functions
// High-level register operations and utility functions
//

#include "arm64_compiler.h"
#include "../../multitasking/fiber/fiber.h"
#include "../../multitasking/thread/sharp_thread.h"

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

void Arm64Compiler::callStaticFunction(void* functionPtr, a64::Gp param1, a64::Gp param2) {
    // Call external C function with 2 parameters
    // ARM64 calling convention: x0 = first param, x1 = second param
    
    // Move parameters to ARM64 calling convention registers
    assembler.mov(a64::x0, param1);  // First parameter
    assembler.mov(a64::x1, param2);  // Second parameter
    
    // Load function address and call
    assembler.mov(tempReg1, reinterpret_cast<uint64_t>(functionPtr));
    assembler.blr(tempReg1);
}

// Jump table helpers

void Arm64Compiler::initializeJumpTable(size_t opcodeCount) {
    // Initialize the opcodeLabels vector with the correct size
    opcodeLabels.clear();
    opcodeLabels.resize(opcodeCount);
    
    // Create labels for each opcode position
    for (size_t i = 0; i < opcodeCount; ++i) {
        opcodeLabels[i] = assembler.newLabel();
    }
}

void Arm64Compiler::setJumpTableEntry(size_t pc, Label opcodeLabel) {
    // Bind the label at this PC position
    if (pc < opcodeLabels.size()) {
        assembler.bind(opcodeLabels[pc]);
    }
}

void Arm64Compiler::generateJumpDispatch(int targetPCRegister) {
    // Generate jump to target PC using jump table
    // jumpTable[targetPC] contains the address to jump to
    // jumpTablePtr should contain the base address of the jump table
    
    // Calculate offset: targetPC * 8 (each pointer is 8 bytes)
    assembler.lsl(tempReg1, a64::Gp::fromTypeAndId(asmjit::RegType::kGp64, targetPCRegister), 3);  // targetPC << 3
    
    // Load jump table address and add offset
    assembler.add(tempReg2, jumpTablePtr, tempReg1);  // jumpTablePtr + (targetPC * 8)
    
    // Load target address from jump table
    assembler.ldr(tempReg3, a64::ptr(tempReg2));      // Load jumpTable[targetPC]
    
    // Jump to target address
    assembler.br(tempReg3);                           // Jump to the target opcode
}

// PC management helpers

void Arm64Compiler::setCurrentPc(a64::Gp targetPCReg) {
    // Update task->pc = base + currentPC + 1
    // Load task pointer
    assembler.ldr(tempReg1, a64::ptr(threadPtr, offsetof(sharp_thread, task)));
    
    // Load base bytecode address from task->rom (bytecode[0])
    assembler.ldr(tempReg2, a64::ptr(tempReg1, offsetof(fiber, rom))); // Load task->rom
    
    // Calculate new PC: base + (currentPC + 1) * sizeof(uint32_t)
    // targetPCReg contains currentPC from caller
    assembler.add(targetPCReg, targetPCReg, 1);      // currentPC + 1
    assembler.lsl(targetPCReg, targetPCReg, 2);      // (currentPC + 1) * 4 (shift left by 2)
    assembler.add(tempReg2, tempReg2, targetPCReg);  // bytecode + ((currentPC + 1) * 4)
    
    // Store updated PC back to task->pc
    assembler.str(tempReg2, a64::ptr(tempReg1, offsetof(fiber, pc)));
}

// Exception handling helpers

void Arm64Compiler::emitExceptionHandle(a64::Gp targetPCReg) {
    // Set the current PC for exception handling context
    // Verify targetPCReg isn't tempReg3, then assign it to tempReg3
    if (targetPCReg.id() != tempReg3.id()) {
        // Move target PC from the provided register to tempReg3
        assembler.mov(tempReg3, targetPCReg);
    }
    
    // Jump to centralized exception handler section
    assembler.b(catchExceptionLabel);
}