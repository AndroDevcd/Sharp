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

bool X64Compiler::emit_ife() {
    /*
     * VM Interpreter Code (virtual_machine.cpp):
     * IFE:
     *     if(regs[CMT]) {
     *         task->pc = task->rom + (Int)regs[ADX];
     *         check_state(0)
     *     } else check_state(1)
     */

    Label taken = assembler->newLabel();
    Label notTaken = assembler->newLabel();

    fldRegister(CMT);
    assembler->fldz();
    assembler->fucomip(x86::st1);
    assembler->fstp(x86::st0);
    assembler->jp(taken);              // NaN counts as true
    assembler->je(notTaken);

    // task->pc = task->rom + (Int)regs[ADX]; check_state(0)
    assembler->bind(taken);
    emitRegisterToInt(ADX, tmp0);
    assembler->mov(pcReg, tmp0);
    emitStateCheck(0);

    // else check_state(1)
    assembler->bind(notTaken);
    emitBranchTo(currentPC + 1);
    return true;
}
