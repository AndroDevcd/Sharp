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

bool X64Compiler::emit_je(int address) {
    /*
     * VM Interpreter Code (virtual_machine.cpp):
     * JE:
     *     if(regs[CMT]) {
     *         task->pc = task->rom + single_arg;
     *         check_state(0)
     *     } else check_state(1)
     */

    Label taken = assembler->newLabel();
    Label notTaken = assembler->newLabel();

    // compare regs[CMT] with 0.0:   st0 = 0.0, st1 = regs[CMT]
    fldRegister(CMT);
    assembler->fldz();
    assembler->fucomip(x86::st1);
    assembler->fstp(x86::st0);

    // if(regs[CMT]) is true for anything that is not 0, including NaN (PF=1)
    assembler->jp(taken);
    assembler->je(notTaken);

    // task->pc = task->rom + address; check_state(0)
    assembler->bind(taken);
    emitBranchTo((size_t)address);

    // else check_state(1)
    assembler->bind(notTaken);
    emitBranchTo(currentPC + 1);
    return true;
}
