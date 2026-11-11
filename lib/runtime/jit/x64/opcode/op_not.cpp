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

bool X64Compiler::emit_not(int outRegister, int inRegister) {
    /*
     * VM Interpreter Code (virtual_machine.cpp):
     * NOT:
     *     regs[dual_arg1] = !regs[dual_arg2];
     *     branch
     */

    // regs[outRegister] = !regs[inRegister]     (1 if it equals 0, otherwise 0)
    fldRegister(inRegister);
    assembler->fldz();
    assembler->fucomip(x86::st1);
    emitSetIfEqual();
    assembler->fstp(x86::st0);
    emitStoreBool(outRegister);
    return true;
}
