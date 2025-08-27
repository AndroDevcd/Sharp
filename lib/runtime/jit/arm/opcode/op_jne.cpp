#include "../arm64_compiler.h"

using namespace asmjit;

bool Arm64Compiler::emit_jne(int address) {
    /*
     * VM Interpreter Code:
     * JNE:
     *     if(regs[CMT] == 0) {
     *         task->pc = task->rom + single_arg;
     *         check_state(0)
     *     } else check_state(1)
     */
    
    // Load CMT register value
    loadRegisterValue(tempVec1, CMT);
    
    // Convert to integer for comparison with zero
    assembler.fcvtzs(tempReg1, tempVec1);  // Convert float to signed integer
    
    // Create labels for conditional branches
    Label jumpToAddress = assembler.newLabel();
    Label continueNext = assembler.newLabel();
    
    // Check if CMT == 0 (condition is false, so we should jump)
    assembler.cbz(tempReg1, jumpToAddress);
    
    // CMT != 0: Don't jump, continue to next instruction
    // check_state(1) means PC += 1, then goto state_check
    emitStateCheck(1);  // Offset of 1 from current PC
    assembler.b(continueNext);
    
    // CMT == 0: Jump to target address
    assembler.bind(jumpToAddress);
    assembler.mov(pcReg, address);  // Set PC to target address
    emitStateCheck(0);  // Always use 0 offset
    
    assembler.bind(continueNext);
    
    return true;
}
