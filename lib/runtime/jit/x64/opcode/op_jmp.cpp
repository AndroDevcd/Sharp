#include "../x64_compiler.h"
#include "../../../multitasking/thread/sharp_thread.h"
#include "../../../multitasking/thread/thread_controller.h"
#include "../../../multitasking/fiber/fiber.h"
#include "../../../memory/vm_stack.h"
#include "../../../memory/sharp_object.h"
#include "../../../types/sharp_class.h"
#include "../../../virtual_machine.h"
#include "../../jit_wrappers.h"
#include "../../../../core/opcode/opcode_macros.h"
#include <cstddef>
#include <cstring>

using namespace asmjit;

bool X64Compiler::emit_jmp(int address) {
    /*
     * VM Interpreter Code (virtual_machine.cpp):
     * JMP:
     *     // A backward jump is the bottom of a loop. Hot loops are worth compiling
     *     // even when the function that contains them is only called once, so every
     *     // JIT_LOOP_MASK+1 trips we try to continue the loop in compiled code.
     *     if(single_arg <= current_pc && (++task->current->loopCount & JIT_LOOP_MASK) == 0 && c_options.jit) {
     *         data = jit_enter_loop(single_arg);
     *         if(data == 2) return;               // the function returned to a native/jit caller
     *         if(data == 1) { check_state(0) }    // compiled code ran, continue wherever task->pc is now
     *     }
     *     task->pc = task->rom + single_arg;
     *     check_state(0)
     */

    // task->pc = task->rom + address; check_state(0)
    emitBranchTo((size_t)address);
    return true;
}
