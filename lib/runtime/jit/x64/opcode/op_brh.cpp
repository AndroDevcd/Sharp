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

bool X64Compiler::emit_brh() {
    /*
     * VM Interpreter Code (virtual_machine.cpp):
     * BRH:
     *     task->pc = task->rom + (Int)regs[ADX];
     *     check_state(0)
     */

    // pcReg = (Int)regs[ADX]; the target is only known at run time so this goes
    // through the bounds-checked jump table dispatch in the state check section
    emitRegisterToInt(ADX, tmp0);
    assembler->mov(pcReg, tmp0);
    emitStateCheck(0);
    return true;
}
