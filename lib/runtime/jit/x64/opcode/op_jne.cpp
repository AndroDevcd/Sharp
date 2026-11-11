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

bool X64Compiler::emit_jne(int address) {
    /*
     * VM Interpreter Code (virtual_machine.cpp):
     * JNE:
     *     if(regs[CMT] == 0) {
     *         task->pc = task->rom + single_arg;
     *         check_state(0)
     *     } else check_state(1)
     */

    Label notZero = assembler->newLabel();

    // compare regs[CMT] with 0.0:   st0 = 0.0, st1 = regs[CMT]
    fldRegister(CMT);
    assembler->fldz();
    assembler->fucomip(x86::st1);
    assembler->fstp(x86::st0);

    // equal sets ZF=1,PF=0. NaN sets ZF=1,PF=1 and NaN == 0 is false in C++.
    assembler->jp(notZero);
    assembler->jne(notZero);

    // regs[CMT] == 0:  task->pc = task->rom + address; check_state(0)
    emitBranchTo((size_t)address);

    // else check_state(1)
    assembler->bind(notZero);
    emitBranchTo(currentPC + 1);
    return true;
}
