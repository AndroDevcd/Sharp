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

bool X64Compiler::emit_ipushl(int relFrameAddress) {
    /*
     * VM Interpreter Code (virtual_machine.cpp):
     * IPUSHL:
     *     grow_stack
     *     stack_overflow_check
     *     push_stack_number = (task->fp + single_arg)->var;
     *     branch
     */

    emitStackCheck(1);

    // tmp2 = (task->fp + relFrameAddress)->var   (raw 8 byte copy of the double)
    loadTask(tmp0);
    assembler->mov(tmp1, x86::qword_ptr(tmp0, offsetof(fiber, fp)));
    assembler->mov(tmp2, stackVar(tmp1, relFrameAddress));

    // (++task->sp)->var = tmp2
    assembler->mov(tmp1, x86::qword_ptr(tmp0, offsetof(fiber, sp)));
    assembler->add(tmp1, (int)sizeof(stack_item));
    assembler->mov(x86::qword_ptr(tmp0, offsetof(fiber, sp)), tmp1);
    assembler->mov(stackVar(tmp1), tmp2);
    return true;
}
