//
// ARM64 JIT Compiler Helper Functions
// High-level register operations and utility functions
//

#include "arm64_compiler.h"
#include "../../multitasking/fiber/fiber.h"
#include "../../multitasking/thread/sharp_thread.h"
#include "../../memory/vm_stack.h"
#include "../jit_wrappers.h"
#include "../../virtual_machine.h"
#include <cstddef>

using namespace asmjit;

// High-level helper functions for VM register operations

/**
 * Load VM register value into ARM64 vector register
 * 
 * REGISTERS USED: registersPtr
 * 
 * @param dest Target ARM64 vector register
 * @param vmReg VM register index
 */
void Arm64Compiler::loadRegisterValue(a64::Vec dest, _register vmReg) {
    // Load registers[vmReg] into dest (floating point register)
    // Each register is 16 bytes (long double), so offset = vmReg * 16
    assembler->ldr(dest, a64::ptr(registersPtr, vmReg * 16));
}

/**
 * Store ARM64 vector register value into VM register
 * 
 * REGISTERS USED: registersPtr
 * 
 * @param vmReg VM register index
 * @param src Source ARM64 vector register  
 */
void Arm64Compiler::storeRegisterValue(_register vmReg, a64::Vec src) {
    // Store src into registers[vmReg] (floating point register)
    assembler->str(src, a64::ptr(registersPtr, vmReg * 16));
}

/**
 * Add two VM registers and store result
 * 
 * REGISTERS USED: tempVec1, tempVec2
 * 
 * @param destReg Destination VM register
 * @param leftReg Left operand VM register
 * @param rightReg Right operand VM register
 */
void Arm64Compiler::addRegisters(_register destReg, _register leftReg, _register rightReg) {
    // registers[destReg] = registers[leftReg] + registers[rightReg]
    loadRegisterValue(tempVec1, leftReg);   // tempVec1 = registers[leftReg]
    loadRegisterValue(tempVec2, rightReg);  // tempVec2 = registers[rightReg]
    assembler->fadd(tempVec1, tempVec1, tempVec2);  // tempVec1 = tempVec1 + tempVec2
    storeRegisterValue(destReg, tempVec1);  // registers[destReg] = tempVec1
}

/**
 * Subtract two VM registers and store result
 * 
 * REGISTERS USED: tempVec1, tempVec2
 * 
 * @param destReg Destination VM register
 * @param leftReg Left operand VM register
 * @param rightReg Right operand VM register
 */
void Arm64Compiler::subRegisters(_register destReg, _register leftReg, _register rightReg) {
    // registers[destReg] = registers[leftReg] - registers[rightReg]
    loadRegisterValue(tempVec1, leftReg);
    loadRegisterValue(tempVec2, rightReg);
    assembler->fsub(tempVec1, tempVec1, tempVec2);
    storeRegisterValue(destReg, tempVec1);
}

/**
 * Multiply two VM registers and store result
 * 
 * REGISTERS USED: tempVec1, tempVec2
 * 
 * @param destReg Destination VM register
 * @param leftReg Left operand VM register
 * @param rightReg Right operand VM register
 */
void Arm64Compiler::mulRegisters(_register destReg, _register leftReg, _register rightReg) {
    // registers[destReg] = registers[leftReg] * registers[rightReg]
    loadRegisterValue(tempVec1, leftReg);
    loadRegisterValue(tempVec2, rightReg);
    assembler->fmul(tempVec1, tempVec1, tempVec2);
    storeRegisterValue(destReg, tempVec1);
}

/**
 * Divide two VM registers and store result
 * 
 * REGISTERS USED: tempVec1, tempVec2
 * 
 * @param destReg Destination VM register
 * @param leftReg Left operand VM register
 * @param rightReg Right operand VM register
 */
void Arm64Compiler::divRegisters(_register destReg, _register leftReg, _register rightReg) {
    // registers[destReg] = registers[leftReg] / registers[rightReg]
    loadRegisterValue(tempVec1, leftReg);
    loadRegisterValue(tempVec2, rightReg);
    assembler->fdiv(tempVec1, tempVec1, tempVec2);
    storeRegisterValue(destReg, tempVec1);
}

/**
 * Copy VM register value to another VM register
 * 
 * REGISTERS USED: tempVec1
 * 
 * @param destReg Destination VM register
 * @param srcReg Source VM register
 */
void Arm64Compiler::moveRegister(_register destReg, _register srcReg) {
    // registers[destReg] = registers[srcReg]
    loadRegisterValue(tempVec1, srcReg);
    storeRegisterValue(destReg, tempVec1);
}

/**
 * Set VM register to immediate integer value
 * 
 * REGISTERS USED: tempReg1, tempVec1
 * 
 * @param vmReg Target VM register
 * @param value Immediate integer value
 */
void Arm64Compiler::setRegisterImmediate(_register vmReg, int64_t value) {
    // registers[vmReg] = immediate value
    assembler->mov(tempReg1, value);
    assembler->scvtf(tempVec1, tempReg1);  // Convert int to double
    storeRegisterValue(vmReg, tempVec1);
}

// Stack operation helpers

/**
 * Pop number value from VM stack
 * 
 * REGISTERS USED: threadPtr, tempReg1, tempReg2
 * 
 * @param destVec Vector register to receive popped value
 */
void Arm64Compiler::popStackNumber(a64::Vec destVec) {
    // Implements: pop_stack_number macro - (task->sp--)->var
    // Load task pointer from thread->task
    assembler->ldr(tempReg1, a64::ptr(threadPtr, offsetof(sharp_thread, task)));
    
    // Load task->sp (stack pointer)
    assembler->ldr(tempReg2, a64::ptr(tempReg1, offsetof(fiber, sp)));
    
    // Pop value from stack: load (task->sp)->var, then decrement sp
    assembler->ldr(destVec, a64::ptr(tempReg2, offsetof(stack_item, var))); // Load stack top value
    assembler->sub(tempReg2, tempReg2, sizeof(stack_item)); // Decrement sp (pop operation)
    assembler->str(tempReg2, a64::ptr(tempReg1, offsetof(fiber, sp))); // Store decremented sp back
}

/**
 * Push number value to VM stack
 * 
 * REGISTERS USED: threadPtr, tempReg1, tempReg2
 * 
 * @param srcVec Vector register containing value to push
 */
void Arm64Compiler::pushStackNumber(a64::Vec srcVec) {
    // Implements: push_stack_number macro - (++task->sp)->var
    // Load task pointer from thread->task
    assembler->ldr(tempReg1, a64::ptr(threadPtr, offsetof(sharp_thread, task)));
    
    // Load task->sp (stack pointer)
    assembler->ldr(tempReg2, a64::ptr(tempReg1, offsetof(fiber, sp)));
    
    // Push value to stack: increment sp, then store value
    assembler->add(tempReg2, tempReg2, sizeof(stack_item)); // Increment sp (push operation)
    assembler->str(tempReg2, a64::ptr(tempReg1, offsetof(fiber, sp))); // Store incremented sp back
    assembler->str(srcVec, a64::ptr(tempReg2, offsetof(stack_item, var))); // Store value at new stack top
}

// External function call helpers

/**
 * Call external C function with no parameters
 * 
 * REGISTERS USED: tempReg1
 * 
 * @param functionPtr Pointer to C function
 */
void Arm64Compiler::callStaticFunction(void* functionPtr) {
    // Call external C function with no parameters
    // Load function address into temp register and call
    assembler->mov(tempReg1, reinterpret_cast<uint64_t>(functionPtr));
    assembler->blr(tempReg1);
}

/**
 * Call external C function with 1 parameter
 * 
 * REGISTERS USED: tempReg1, returnReg (x0 for parameter passing)
 * 
 * @param functionPtr Pointer to C function
 * @param param1 First parameter register
 */
void Arm64Compiler::callStaticFunction(void* functionPtr, a64::Gp param1) {
    // Call external C function with 1 parameter
    // ARM64 calling convention: x0 = first param
    
    // Move parameter to ARM64 calling convention register
    assembler->mov(a64::x0, param1);  // First parameter
    
    // Load function address and call
    assembler->mov(tempReg1, reinterpret_cast<uint64_t>(functionPtr));
    assembler->blr(tempReg1);
}

/**
 * Call external C function with 2 parameters
 * 
 * REGISTERS USED: tempReg1, returnReg (x0), a64::x1 (for parameter passing)
 * 
 * @param functionPtr Pointer to C function
 * @param param1 First parameter register
 * @param param2 Second parameter register
 */
void Arm64Compiler::callStaticFunction(void* functionPtr, a64::Gp param1, a64::Gp param2) {
    // Call external C function with 2 parameters
    // ARM64 calling convention: x0 = first param, x1 = second param
    
    // Move parameters to ARM64 calling convention registers
    assembler->mov(a64::x0, param1);  // First parameter
    assembler->mov(a64::x1, param2);  // Second parameter
    
    // Load function address and call
    assembler->mov(tempReg1, reinterpret_cast<uint64_t>(functionPtr));
    assembler->blr(tempReg1);
}

/**
 * Call C++ instance method with 1 parameter
 * 
 * REGISTERS USED: tempReg1, returnReg (x0), a64::x1 (for parameter passing)
 * 
 * @param functionPtr Pointer to C++ method
 * @param instance This pointer register
 * @param param1 First parameter register
 */
void Arm64Compiler::callInstanceFunction(void* functionPtr, a64::Gp instance, a64::Gp param1) {
    // Call C++ instance method with 1 parameter
    // ARM64 calling convention: x0 = this pointer, x1 = first param
    
    // Move parameters to ARM64 calling convention registers
    assembler->mov(a64::x0, instance);  // 'this' pointer
    assembler->mov(a64::x1, param1);    // First parameter
    
    // Load function address and call
    assembler->mov(tempReg1, reinterpret_cast<uint64_t>(functionPtr));
    assembler->blr(tempReg1);
}

// Jump table helpers

void Arm64Compiler::initializeJumpTable(size_t opcodeCount) {
    // Initialize the opcodeLabels vector with the correct size
    opcodeLabels.clear();
    opcodeLabels.resize(opcodeCount);
    
    // Create labels for each opcode position
    for (size_t i = 0; i < opcodeCount; ++i) {
        opcodeLabels[i] = assembler->newLabel();
    }
}

void Arm64Compiler::setJumpTableEntry(size_t pc, Label opcodeLabel) {
    // Bind the label at this PC position
    if (pc < opcodeLabels.size()) {
        assembler->bind(opcodeLabels[pc]);
    }
}

/**
 * Generate jump to target PC using jump table with bounds checking
 * 
 * REGISTERS USED: tempReg1, tempReg2, tempReg3, jumpTablePtr
 * 
 * @param targetPc Target PC to jump to
 */
void Arm64Compiler::generateJumpDispatch(int targetPc) {
    // Generate jump to target PC using jump table with bounds checking
    // jumpTable[targetPC] contains the address to jump to
    // jumpTableSize equals currentFunction->bytecodeSize (number of opcodes)
    
    // Load targetPC into register for bounds checking
    assembler->mov(tempReg1, targetPc);
    
    // Bounds check: if (targetPC < 0) goto illegal_branch
    assembler->cmp(tempReg1, 0);
    assembler->b_lt(illegalBranchLabel);
    
    // Bounds check: if (targetPC >= bytecodeSize) goto illegal_branch
    assembler->cmp(tempReg1, currentFunction->bytecodeSize);
    assembler->b_ge(illegalBranchLabel);
    
    // Bounds check passed - proceed with jump dispatch
    // Calculate offset: targetPC * 8 (each pointer is 8 bytes)
    assembler->lsl(tempReg1, tempReg1, 3);  // targetPC << 3
    
    // Load jump table address and add offset
    assembler->add(tempReg2, jumpTablePtr, tempReg1);  // jumpTablePtr + (targetPC * 8)
    
    // Load target address from jump table
    assembler->ldr(tempReg3, a64::ptr(tempReg2));      // Load jumpTable[targetPC]
    
    // Jump to target address
    assembler->br(tempReg3);                           // Jump to the target opcode
}

// PC management helpers
/**
 * Store current PC value to task->pc
 * 
 * REGISTERS USED: threadPtr, pcReg, tempReg1, tempReg2, tempReg3
 * 
 */
void Arm64Compiler::storePC() { // [x]
    // Update task->pc = base + pcReg
    // Load task pointer
    assembler->ldr(tempReg1, a64::ptr(threadPtr, offsetof(sharp_thread, task)));
    
    // Load base bytecode address from task->rom (bytecode[0])
    assembler->ldr(tempReg2, a64::ptr(tempReg1, offsetof(fiber, rom))); // Load task->rom
    
    // Calculate PC pointer: base + (pcReg * sizeof(uint32_t))
    assembler->lsl(tempReg3, pcReg, 2);               // pcReg * 4 (shift left by 2)
    assembler->add(tempReg2, tempReg2, tempReg3);     // bytecode + (pcReg * 4)
    
    // Store updated PC back to task->pc
    assembler->str(tempReg2, a64::ptr(tempReg1, offsetof(fiber, pc)));
}

/**
 * Load numeric PC from task->pc pointer
 * 
 * REGISTERS USED: threadPtr, tempReg1, tempReg2, tempReg3, pcReg
 * 
 */
void Arm64Compiler::loadPC() { // [x]
    // Load numeric PC from task->pc pointer based on relation to base pointer
    // Load task pointer
    assembler->ldr(tempReg1, a64::ptr(threadPtr, offsetof(sharp_thread, task)));
    
    // Load current PC pointer from task->pc
    assembler->ldr(tempReg2, a64::ptr(tempReg1, offsetof(fiber, pc)));
    
    // Load base bytecode address from task->rom (bytecode[0])
    assembler->ldr(tempReg3, a64::ptr(tempReg1, offsetof(fiber, rom)));
    
    // Calculate PC offset: (task->pc - base)
    assembler->sub(tempReg2, tempReg2, tempReg3);     // PC pointer - base
    
    // Convert to instruction index: offset / sizeof(uint32_t)
    assembler->lsr(pcReg, tempReg2, 2);               // (PC pointer - base) / 4
}

// Exception handling helpers

void Arm64Compiler::emitExceptionHandle() {
    // Jump to centralized exception handler section
    assembler->b(catchExceptionLabel);
}

/**
 * Fast exception check after function calls
 * 
 * REGISTERS USED: threadPtr, checkReg (parameter)
 * 
 * @param checkReg Register to use for exception checking
 */
void Arm64Compiler::emitFastExceptionCheck(a64::Gp checkReg) {
    /*
     * Fast exception check after function calls
     * Based on growStack exception checking pattern:
     * 
     * // Load thread->signal and check tsig_except (bit 0)
     * assembler->ldr(checkReg, a64::ptr(threadPtr, offsetof(sharp_thread, signal)));
     * assembler->tbz(checkReg, 0, exceptionOk);  // Test bit 0, jump to exceptionOk if no exception
     * 
     * // Exception occurred - jump to exception handler
     * emitExceptionHandle();
     * 
     * // Safety measure in case exception handler returns
     * emitReturn(JIT_OK);
     * 
     * // No exception - continue execution
     * assembler->bind(exceptionOk);
     */
    
    // Create label for successful (no exception) case
    Label exceptionOk = assembler->newLabel();
    
    // Load thread->signal and check tsig_except (bit 0)
    assembler->ldr(checkReg, a64::ptr(threadPtr, offsetof(sharp_thread, signal)));
    assembler->tbz(checkReg, 0, exceptionOk);  // Test bit 0, jump to exceptionOk if no exception
    
    // Exception occurred - jump to exception handler
    emitExceptionHandle();
    
    // Safety measure in case exception handler returns (should never get here)
    emitReturn(JIT_OK);
    
    // No exception - continue execution
    assembler->bind(exceptionOk);
}

// VM Stack Operation Helpers - Centralized Sections

/**
 * Emit jump to centralized grow stack section
 * 
 * REGISTERS USED: tempReg1, tempReg2
 * 
 * @param n Number of items to check for stack growth
 * @param returnLabel Label to return to after stack check
 */
void Arm64Compiler::emitGrowStackCheck(int n, Label &returnLabel) { // [x]
    /*
     * Emit jump to centralized grow stack section
     * Parameters passed in registers:
     * - tempReg1: n (number of items to check)  
     * - tempReg2: return address (label address)
     */
    
    // Store n parameter in tempReg1
    assembler->mov(tempReg1, n);
    
    // Store return label address in tempReg2
    assembler->adr(tempReg2, returnLabel);
    
    // Jump to centralized grow stack section
    assembler->b(growStackLabel);
    
    // Bind the return label immediately after the call
    assembler->bind(returnLabel);
}

/**
 * Emit jump to centralized stack overflow section
 * 
 * REGISTERS USED: tempReg1, tempReg2
 * 
 * @param n Number of items to check for stack overflow
 * @param returnLabel Label to return to after stack check
 */
void Arm64Compiler::emitStackOverflowCheck(int n, Label &returnLabel) {
    /*
     * Emit jump to centralized stack overflow section  
     * Parameters passed in registers:
     * - tempReg1: n (number of items to check)
     * - tempReg2: return address (label address)
     */
    
    // Store n parameter in tempReg1
    assembler->mov(tempReg1, n);
    
    // Store return label address in tempReg2
    assembler->adr(tempReg2, returnLabel);
    
    // Jump to centralized stack overflow section
    assembler->b(stackOverflowLabel);
    
    // Bind the return label immediately after the call
    assembler->bind(returnLabel);
}

/**
 * Generate centralized grow stack section with exception checking
 * 
 * REGISTERS USED: threadPtr, tempReg1, tempReg2, tempReg3, tempReg4, tempReg5, tempReg6, tempReg7, tempReg8, returnReg
 * 
 */
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
    
    assembler->bind(growStackLabel);
    
    // Preserve input registers immediately using new temp registers to avoid conflicts
    assembler->mov(tempReg6, tempReg2);     // Save return address in tempReg6
    assembler->mov(tempReg7, tempReg1);     // Save n parameter in tempReg7
    
    // Get task pointer: task = thread->task
    assembler->ldr(tempReg1, a64::ptr(threadPtr, offsetof(sharp_thread, task)));
    
    // Load task->sp and task->stack using separate registers
    assembler->ldr(tempReg2, a64::ptr(tempReg1, offsetof(fiber, sp)));     // task->sp
    assembler->ldr(tempReg3, a64::ptr(tempReg1, offsetof(fiber, stack)));  // task->stack
    
    // Calculate current stack depth: (task->sp - task->stack)
    assembler->sub(tempReg4, tempReg2, tempReg3);
    
    // Convert to item count: depth / sizeof(stack_item)
    assembler->mov(tempReg5, sizeof(stack_item));
    assembler->udiv(tempReg4, tempReg4, tempReg5);
    
    // Add n to current depth: (current_depth + n) [n is in tempReg7]
    assembler->add(tempReg4, tempReg4, tempReg7);
    
    // Load task->stackSize
    assembler->ldr(tempReg8, a64::ptr(tempReg1, offsetof(fiber, stackSize)));
    
    // Compare: if ((current_depth + n) >= stackSize)
    Label growStackOk = assembler->newLabel();
    assembler->cmp(tempReg4, tempReg8);
    assembler->b_lt(growStackOk);
    
    // Need to grow stack - first store PC
    storePC();
    
    // Set up parameters for jit_growStack(task, n)
    // x0 = task (already in tempReg1)
    // x1 = n (in tempReg7)
    callStaticFunction(reinterpret_cast<void*>(jit_growStack), tempReg1, tempReg7);
    
    // Fast exception check after growStack call using fresh register
    emitFastExceptionCheck(tempReg2);
    
    // Stack growth is OK - return to caller
    assembler->bind(growStackOk);
    assembler->br(tempReg6);  // Jump back to return address (saved in tempReg6)
}

/**
 * Generate centralized stack overflow check section
 * 
 * REGISTERS USED: threadPtr, tempReg1, tempReg2, tempReg3, tempReg4, tempReg5, returnReg
 * 
 */
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
    
    assembler->bind(stackOverflowLabel);
    
    // Get task pointer: task = thread->task
    assembler->ldr(tempReg3, a64::ptr(threadPtr, offsetof(sharp_thread, task)));
    
    // Load task->sp and task->stack
    assembler->ldr(tempReg4, a64::ptr(tempReg3, offsetof(fiber, sp)));
    assembler->ldr(returnReg, a64::ptr(tempReg3, offsetof(fiber, stack)));
    
    // Calculate current stack depth: (task->sp - task->stack)
    assembler->sub(tempReg4, tempReg4, returnReg);
    
    // Convert to item count: depth / sizeof(stack_item)
    assembler->mov(tempReg5, sizeof(stack_item));
    assembler->udiv(tempReg4, tempReg4, tempReg5);
    
    // Add n to current depth: (current_depth + n) [n is in tempReg1]
    assembler->add(tempReg4, tempReg4, tempReg1);
    
    // Load task->stackLimit
    assembler->ldr(returnReg, a64::ptr(tempReg3, offsetof(fiber, stackLimit)));
    
    // Compare: if ((current_depth + n) >= stackLimit) throw exception
    Label stackOverflowOk = assembler->newLabel();
    assembler->cmp(tempReg4, returnReg);
    assembler->b_lt(stackOverflowOk);
    storePC();
    
    // Stack overflow - create vm_exception(vm.stack_overflow_except, "")
    // Load vm.stack_overflow_except class pointer (it's already a sharp_class*)
    // Call jit_throwException(vm.stack_overflow_except, "")
    assembler->mov(a64::x0, reinterpret_cast<uint64_t>(vm.stack_overflow_except));
    assembler->mov(a64::x1, reinterpret_cast<uint64_t>(""));  // empty message
    callStaticFunction(reinterpret_cast<void*>(jit_throwException), a64::x0, a64::x1);
    
    // After exception is created, jump to exception handler
    emitExceptionHandle();

    // should never get here but just a saftey measure
    emitReturn(JIT_OK);
    
    // Stack is OK - return to caller
    assembler->bind(stackOverflowOk);
    assembler->br(tempReg2);  // Jump back to return address
}

/**
 * Push immediate double value to VM stack
 * 
 * REGISTERS USED: threadPtr, tempReg1, tempReg2, tempReg3, tempVec1
 * 
 * @param value Double value to push
 */
void Arm64Compiler::emitPushStackNumber(double value) {
    /*
     * VM Interpreter Macro:
     * #define push_stack_number (++task->sp)->var
     * Equivalent to: (++task->sp)->var = value;
     */
    
    // Get task pointer: task = thread->task
    assembler->ldr(tempReg1, a64::ptr(threadPtr, offsetof(sharp_thread, task)));
    
    // Load current stack pointer: sp = task->sp
    assembler->ldr(tempReg2, a64::ptr(tempReg1, offsetof(fiber, sp)));
    
    // Increment stack pointer: ++task->sp
    assembler->add(tempReg2, tempReg2, sizeof(stack_item));
    
    // Store incremented sp back to task->sp
    assembler->str(tempReg2, a64::ptr(tempReg1, offsetof(fiber, sp)));
    
    // Convert double value to vector register and store in stack_item.var
    assembler->fmov(tempVec1, value);  // Load immediate double value
    
    // Store the double value in stack_item.var field (offset 0)
    assembler->str(tempVec1, a64::ptr(tempReg2, offsetof(stack_item, var)));
}

/**
 * Push immediate integer value to VM stack (converted to double)
 * 
 * REGISTERS USED: threadPtr, tempReg1, tempReg2, tempReg3, tempVec1
 * 
 * @param intValue Integer value to convert and push
 */
void Arm64Compiler::emitPushStackNumberImmediate(int32_t intValue) {
    /*
     * VM Interpreter Macro:
     * #define push_stack_number (++task->sp)->var
     * For: push_stack_number = raw_arg2; (where raw_arg2 is int32_t)
     */
    
    // Get task pointer: task = thread->task
    assembler->ldr(tempReg1, a64::ptr(threadPtr, offsetof(sharp_thread, task)));
    
    // Load current stack pointer: sp = task->sp
    assembler->ldr(tempReg2, a64::ptr(tempReg1, offsetof(fiber, sp)));
    
    // Increment stack pointer: ++task->sp
    assembler->add(tempReg2, tempReg2, sizeof(stack_item));
    
    // Store incremented sp back to task->sp
    assembler->str(tempReg2, a64::ptr(tempReg1, offsetof(fiber, sp)));
    
    // Convert int32 immediate to double and store in stack_item.var
    assembler->mov(tempReg3, intValue);        // Load immediate int value
    assembler->scvtf(tempVec1, tempReg3);      // Convert int to double
    
    // Store the double value in stack_item.var field (offset 0)
    assembler->str(tempVec1, a64::ptr(tempReg2, offsetof(stack_item, var)));
}

/**
 * Push stack object and return address for assignment
 * 
 * REGISTERS USED: threadPtr, tempReg1, tempReg2
 * 
 * @param destReg Register to receive address of new stack object
 */
void Arm64Compiler::emitPushStackObject(a64::Gp destReg) {
    /*
     * VM Interpreter Macro:
     * #define push_stack_object (++task->sp)->obj
     * 
     * This function:
     * 1. Increments task->sp (push operation)
     * 2. Returns the address of the obj field in destReg: &((++task->sp)->obj)
     */
    
    // Get task pointer: task = thread->task
    assembler->ldr(tempReg1, a64::ptr(threadPtr, offsetof(sharp_thread, task)));
    
    // Load current stack pointer: sp = task->sp
    assembler->ldr(tempReg2, a64::ptr(tempReg1, offsetof(fiber, sp)));
    
    // Increment stack pointer: ++task->sp
    assembler->add(tempReg2, tempReg2, sizeof(stack_item));
    
    // Store incremented sp back to task->sp
    assembler->str(tempReg2, a64::ptr(tempReg1, offsetof(fiber, sp)));
    
    // Calculate address of obj field: &(task->sp)->obj
    assembler->add(destReg, tempReg2, offsetof(stack_item, obj));
}

/**
 * Pop stack object and return address of popped object
 * 
 * REGISTERS USED: threadPtr, tempReg1, tempReg2
 * 
 * @param destReg Register to receive address of popped stack object
 */
void Arm64Compiler::emitPopStackObject(a64::Gp destReg) {
    /*
     * VM Interpreter Macro:
     * #define pop_stack_object (task->sp--)->obj
     * 
     * This function:
     * 1. Gets the address of the current stack top's obj field
     * 2. Decrements task->sp (pop operation)
     * 3. Returns the address of the obj field before decrement: &((task->sp--)->obj)
     */
    
    // Get task pointer: task = thread->task
    assembler->ldr(tempReg1, a64::ptr(threadPtr, offsetof(sharp_thread, task)));
    
    // Load current stack pointer: sp = task->sp
    assembler->ldr(tempReg2, a64::ptr(tempReg1, offsetof(fiber, sp)));
    
    // Calculate address of current obj field before decrement: &(task->sp)->obj
    assembler->add(destReg, tempReg2, offsetof(stack_item, obj));
    
    // Decrement stack pointer: task->sp--
    assembler->sub(tempReg2, tempReg2, sizeof(stack_item));
    
    // Store decremented sp back to task->sp
    assembler->str(tempReg2, a64::ptr(tempReg1, offsetof(fiber, sp)));
}

/**
 * Jump to centralized object value check section
 * 
 * REGISTERS USED: tempReg2
 * 
 * @param returnLabel Label to return to after successful validation
 */
void Arm64Compiler::emitRequireObjectWithValue(Label &returnLabel) {
    /*
     * Jump to centralized object value check section
     * Parameters passed in registers:
     * - tempReg2: return address (label address)
     */
    
    // Store return label address in tempReg2
    assembler->adr(tempReg2, returnLabel);
    
    // Jump to centralized object value check section
    assembler->b(objectValueCheckLabel);
    
    // Bind the return label immediately after the call
    assembler->bind(returnLabel);
}

/**
 * Jump to centralized numeric object check section
 * 
 * REGISTERS USED: tempReg2
 * 
 * @param returnLabel Label to return to after successful validation
 */
void Arm64Compiler::emitRequireNumericObjectWithValue(Label &returnLabel) {
    /*
     * Jump to centralized numeric object check section
     * Parameters passed in registers:
     * - tempReg2: return address (label address)
     */
    
    // Store return label address in tempReg2
    assembler->adr(tempReg2, returnLabel);
    
    // Jump to centralized numeric object check section
    assembler->b(numericObjectCheckLabel);
    
    // Bind the return label immediately after the call
    assembler->bind(returnLabel);
}