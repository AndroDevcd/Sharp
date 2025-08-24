//
// ARM64 JIT Compiler - INT Opcode Implementation
//

#include "../arm64_compiler.h"

using namespace asmjit;

bool Arm64Compiler::emit_int(int flag) {
    /*
     * VM Interpreter Code:
     * _INT:
     *     exec_interrupt(single_arg);
     *     if(vm.state == VM_TERMINATED) {
     *         send_interrupt_signal(thread);
     *         return;
     *     }
     *     check_state(1)
     */
    
    // TODO: Implement INT opcode
    return false;
}