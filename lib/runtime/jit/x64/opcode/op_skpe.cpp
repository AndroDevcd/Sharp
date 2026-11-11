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

bool X64Compiler::emit_skpe(int inRegister, int address) {
    /*
     * VM Interpreter Code (virtual_machine.cpp):
     * SKPE: // todo: phase out
     *     if((Int)regs[dual_arg1] != 0) {
     *         task->pc = task->pc + dual_arg2;
     *         check_state(0)
     *     } else check_state(1)
     */

    // if((Int)regs[inRegister] != 0) { task->pc += address; check_state(0) } else check_state(1)
    Label isZero = assembler->newLabel();
    emitRegisterToInt(inRegister, tmp0);
    assembler->test(tmp0, tmp0);
    assembler->jz(isZero);
    emitBranchTo(currentPC + address);
    assembler->bind(isZero);
    emitBranchTo(currentPC + 1);
    return true;
}
