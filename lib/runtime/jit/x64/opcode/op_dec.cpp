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

bool X64Compiler::emit_dec(int outRegister) {
    /*
     * VM Interpreter Code (virtual_machine.cpp):
     * DEC:
     *     regs[single_arg]--;
     *     branch
     */

    // regs[outRegister]--
    fldRegister(outRegister);
    assembler->fld1();
    assembler->fsubp(x86::st1);                // st1 = st1 - st0
    fstpRegister(outRegister);
    return true;
}
