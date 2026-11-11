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

bool X64Compiler::emit_sub(int outRegister, int leftRegister, int rightRegister) {
    /*
     * VM Interpreter Code (virtual_machine.cpp):
     * SUB:
     *     regs[triple_arg3] = regs[triple_arg1] - regs[triple_arg2];
     *     branch
     */

    // regs[outRegister] = regs[leftRegister] - regs[rightRegister]
    fldRegister(leftRegister);                 // st0 = left
    fldRegister(rightRegister);                // st0 = right, st1 = left
    assembler->fsubp(x86::st1);                // st1 = st1 - st0, pop -> st0 = result
    fstpRegister(outRegister);
    return true;
}
