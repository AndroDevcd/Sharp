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

bool X64Compiler::emit_mov8(int outRegister, int registerToCast) {
    /*
     * VM Interpreter Code (virtual_machine.cpp):
     * MOV8:
     *     regs[dual_arg1] = (int8_t)regs[dual_arg2];
     *     branch
     */

    // regs[outRegister] = (int8_t)regs[registerToCast]
    emitRegisterToInt(registerToCast, tmp0);   // truncate to a 64-bit integer first
    assembler->movsx(tmp0, x86::al);                      // keep the low 8 bits, sign extended
    assembler->mov(scratch(8), tmp0);
    assembler->fild(scratch(8));
    fstpRegister(outRegister);
    return true;
}
