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

bool X64Compiler::emit_rstore(int registerToCast) {
    /*
     * VM Interpreter Code (virtual_machine.cpp):
     * RSTORE:
     *     grow_stack
     *     stack_overflow_check
     *     push_stack_number = regs[single_arg];
     *     branch
     */

    emitStackCheck(1);

    // (++task->sp)->var = regs[registerToCast]
    emitPushSlot(tmp1);
    fldRegister(registerToCast);
    assembler->fstp(stackVar(tmp1));
    return true;
}
