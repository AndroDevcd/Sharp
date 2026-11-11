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

bool X64Compiler::emit_movi(int value, int outRegister) {
    /*
     * VM Interpreter Code (virtual_machine.cpp):
     * _MOVI:
     *     regs[single_arg] = raw_arg2;
     *     branch_for(2)
     */

    // regs[outRegister] = (long double)value
    emitLoadInt(value);
    fstpRegister(outRegister);
    return true;
}
