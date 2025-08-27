//
// ARM64 JIT Compiler Helper Functions
// High-level register operations and utility functions
//

#include "arm64_compiler.h"
#include "../../multitasking/fiber/fiber.h"
#include "../../multitasking/thread/sharp_thread.h"
#include "../jit_wrappers.h"

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

void Arm64Compiler::callInstanceFunction(void* functionPtr, a64::Gp instance, a64::Gp param1) {
    // Call C++ instance method with 1 parameter
    // ARM64 calling convention: x0 = this pointer, x1 = first param
    
    // Move parameters to ARM64 calling convention registers
    assembler.mov(a64::x0, instance);  // 'this' pointer
    assembler.mov(a64::x1, param1);    // First parameter
    
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

// VM Stack Operation Helpers - Centralized Sections

void Arm64Compiler::emitGrowStackCheck(int n, Label returnLabel) {
    /*
     * Emit jump to centralized grow stack section
     * Parameters passed in registers:
     * - tempReg1: n (number of items to check)  
     * - tempReg2: return address (label address)
     */
    
    // Store n parameter in tempReg1
    assembler.mov(tempReg1, n);
    
    // Store return label address in tempReg2
    assembler.adr(tempReg2, returnLabel);
    
    // Jump to centralized grow stack section
    assembler.b(growStackLabel);
    
    // Bind the return label immediately after the call
    assembler.bind(returnLabel);
}

void Arm64Compiler::emitStackOverflowCheck(int n, Label returnLabel) {
    /*
     * Emit jump to centralized stack overflow section  
     * Parameters passed in registers:
     * - tempReg1: n (number of items to check)
     * - tempReg2: return address (label address)
     */
    
    // Store n parameter in tempReg1
    assembler.mov(tempReg1, n);
    
    // Store return label address in tempReg2
    assembler.adr(tempReg2, returnLabel);
    
    // Jump to centralized stack overflow section
    assembler.b(stackOverflowLabel);
    
    // Bind the return label immediately after the call
    assembler.bind(returnLabel);
}

void Arm64Compiler::generateGrowStackSection() {
    /*
     * Centralized grow stack section
     * VM Interpreter Macro:
     * #define grow_stack_for(n) 
     *     if(((task->sp-task->stack)+(n)) >= task->stackSize) task->growStack((n));
     * 
     * Input registers:
     * - tempReg1: n (number of items to grow)
     * - tempReg2: return address to jump back to
     */
    
    assembler.bind(growStackLabel);
    
    // Get task pointer: task = thread->task
    assembler.ldr(tempReg3, a64::ptr(threadPtr, offsetof(sharp_thread, task)));
    
    // Load task->sp and task->stack  
    assembler.ldr(tempReg4, a64::ptr(tempReg3, offsetof(fiber, sp)));
    assembler.ldr(returnReg, a64::ptr(tempReg3, offsetof(fiber, stack)));
    
    // Calculate current stack depth: (task->sp - task->stack)
    assembler.sub(tempReg4, tempReg4, returnReg);
    
    // Convert to item count: depth / sizeof(stack_item)
    assembler.mov(returnReg, sizeof(stack_item));
    assembler.udiv(tempReg4, tempReg4, returnReg);
    
    // Add n to current depth: (current_depth + n) [n is in tempReg1]
    assembler.add(tempReg4, tempReg4, tempReg1);
    
    // Load task->stackSize
    assembler.ldr(returnReg, a64::ptr(tempReg3, offsetof(fiber, stackSize)));
    
    // Compare: if ((current_depth + n) >= stackSize)
    Label growStackOk = assembler.newLabel();
    assembler.cmp(tempReg4, returnReg);
    assembler.b_lt(growStackOk);
    
    // Need to grow stack - call task->growStack(n)
    // Set up parameters: x0 = task, x1 = n
    assembler.mov(a64::x0, tempReg3);    // task pointer
    assembler.mov(a64::x1, tempReg1);    // n parameter
    
    // Call task->growStack(n) using the wrapper function
    // We have: task instance in tempReg3, parameter n in tempReg1
    
    // Call the wrapper function: jit_growStack(task, n)
    callStaticFunction(reinterpret_cast<void*>(jit_growStack), tempReg3, tempReg1);
    
    // Stack growth is OK - return to caller
    assembler.bind(growStackOk);
    assembler.br(tempReg2);  // Jump back to return address
}

void Arm64Compiler::generateStackOverflowSection() {
    /*
     * Centralized stack overflow check section
     * VM Interpreter Macro:
     * #define stack_overflow_check_for(n)  
     *     if(((task->sp-task->stack)+(n)) >= task->stackLimit) throw vm_exception(vm.stack_overflow_except, "");
     * 
     * Input registers:
     * - tempReg1: n (number of items to check)
     * - tempReg2: return address to jump back to
     */
    
    assembler.bind(stackOverflowLabel);
    
    // Get task pointer: task = thread->task
    assembler.ldr(tempReg3, a64::ptr(threadPtr, offsetof(sharp_thread, task)));
    
    // Load task->sp and task->stack
    assembler.ldr(tempReg4, a64::ptr(tempReg3, offsetof(fiber, sp)));
    assembler.ldr(returnReg, a64::ptr(tempReg3, offsetof(fiber, stack)));
    
    // Calculate current stack depth: (task->sp - task->stack)
    assembler.sub(tempReg4, tempReg4, returnReg);
    
    // Convert to item count: depth / sizeof(stack_item)
    assembler.mov(returnReg, sizeof(stack_item));
    assembler.udiv(tempReg4, tempReg4, returnReg);
    
    // Add n to current depth: (current_depth + n) [n is in tempReg1]
    assembler.add(tempReg4, tempReg4, tempReg1);
    
    // Load task->stackLimit
    assembler.ldr(returnReg, a64::ptr(tempReg3, offsetof(fiber, stackLimit)));
    
    // Compare: if ((current_depth + n) >= stackLimit) throw exception
    Label stackOverflowOk = assembler.newLabel();
    assembler.cmp(tempReg4, returnReg);
    assembler.b_lt(stackOverflowOk);
    
    // Stack overflow - throw exception
    // todo: implement low level exception throwing and handeling
    emitReturn(JIT_EXCEPTION);
    
    // Stack is OK - return to caller
    assembler.bind(stackOverflowOk);
    assembler.br(tempReg2);  // Jump back to return address
}

void Arm64Compiler::emitPushStackNumber(double value) {
    /*
     * VM Interpreter Macro:
     * #define push_stack_number (++task->sp)->var
     * Equivalent to: (++task->sp)->var = value;
     */
    
    // Get task pointer: task = thread->task
    assembler.ldr(tempReg1, a64::ptr(threadPtr, offsetof(sharp_thread, task)));
    
    // Load current stack pointer: sp = task->sp
    assembler.ldr(tempReg2, a64::ptr(tempReg1, offsetof(fiber, sp)));
    
    // Increment stack pointer: ++task->sp
    assembler.add(tempReg2, tempReg2, sizeof(stack_item));
    
    // Store incremented sp back to task->sp
    assembler.str(tempReg2, a64::ptr(tempReg1, offsetof(fiber, sp)));
    
    // Convert double value to vector register and store in stack_item.var
    assembler.fmov(tempVec1, value);  // Load immediate double value
    
    // Store the double value in stack_item.var field (offset 0)
    assembler.str(tempVec1, a64::ptr(tempReg2, offsetof(stack_item, var)));
}

void Arm64Compiler::emitPushStackNumberImmediate(int32_t intValue) {
    /*
     * VM Interpreter Macro:
     * #define push_stack_number (++task->sp)->var
     * For: push_stack_number = raw_arg2; (where raw_arg2 is int32_t)
     */
    
    // Get task pointer: task = thread->task
    assembler.ldr(tempReg1, a64::ptr(threadPtr, offsetof(sharp_thread, task)));
    
    // Load current stack pointer: sp = task->sp
    assembler.ldr(tempReg2, a64::ptr(tempReg1, offsetof(fiber, sp)));
    
    // Increment stack pointer: ++task->sp
    assembler.add(tempReg2, tempReg2, sizeof(stack_item));
    
    // Store incremented sp back to task->sp
    assembler.str(tempReg2, a64::ptr(tempReg1, offsetof(fiber, sp)));
    
    // Convert int32 immediate to double and store in stack_item.var
    assembler.mov(tempReg3, intValue);        // Load immediate int value
    assembler.scvtf(tempVec1, tempReg3);      // Convert int to double
    
    // Store the double value in stack_item.var field (offset 0)
    assembler.str(tempVec1, a64::ptr(tempReg2, offsetof(stack_item, var)));
}