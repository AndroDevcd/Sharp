#include "../arm64_compiler.h"

using namespace asmjit;

bool Arm64Compiler::emit_jmp(int address) {
    /*
     * VM Interpreter Code:
     * JMP:
     *     task->pc = task->rom + single_arg;
     *     check_state(0)
     */
    
    // Set PC to target address and emit state check
    assembler.mov(pcReg, address);  // Set PC to target address
    emitStateCheck(0);  // Always use 0 offset since PC is already set
    
    return true;
}
