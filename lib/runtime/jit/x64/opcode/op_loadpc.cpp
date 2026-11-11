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

bool X64Compiler::emit_loadpc(int outRegister) {
    /*
     * VM Interpreter Code (virtual_machine.cpp):
     * LOADPC:
     *     regs[single_arg] = current_pc;
     *     branch
     */

    // regs[outRegister] = current_pc      (known while compiling)
    emitLoadInt((int)currentPC);
    fstpRegister(outRegister);
    return true;
}
