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

bool X64Compiler::emit_loadval(int outRegister) {
    /*
     * VM Interpreter Code (virtual_machine.cpp):
     * LOADVAL:
     *     regs[single_arg] = pop_stack_number;
     *     branch
     */

    // regs[outRegister] = (task->sp--)->var
    emitPopSlot(tmp1);
    assembler->fld(stackVar(tmp1));
    fstpRegister(outRegister);
    return true;
}
