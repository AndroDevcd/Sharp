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

bool X64Compiler::emit_cmp(int inRegister, int value) {
    /*
     * VM Interpreter Code (virtual_machine.cpp):
     * CMP:
     *     regs[CMT] = regs[single_arg] == raw_arg2;
     *     branch_for(2)
     */

    // regs[CMT] = regs[inRegister] == value
    fldRegister(inRegister);
    emitLoadInt(value);                        // st0 = value, st1 = regs[inRegister]
    assembler->fucomip(x86::st1);
    emitSetIfEqual();
    assembler->fstp(x86::st0);
    emitStoreBool(CMT);
    return true;
}
