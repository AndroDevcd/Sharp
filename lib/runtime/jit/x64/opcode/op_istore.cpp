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

bool X64Compiler::emit_istore(int value) {
    /*
     * VM Interpreter Code (virtual_machine.cpp):
     * ISTORE:
     *     grow_stack
     *     stack_overflow_check
     *     push_stack_number = raw_arg2;
     *     branch_for(2)
     */

    emitStackCheck(1);

    // tmp1 = ++task->sp
    loadTask(tmp0);
    assembler->mov(tmp1, x86::qword_ptr(tmp0, offsetof(fiber, sp)));
    assembler->add(tmp1, (int)sizeof(stack_item));
    assembler->mov(x86::qword_ptr(tmp0, offsetof(fiber, sp)), tmp1);

    // tmp1->var = (double)value   (the int -> double conversion is done right now,
    // at compile time, and the resulting bit pattern is stored)
    double number = (double)value;
    uint64_t bits;
    memcpy(&bits, &number, sizeof(bits));
    assembler->mov(tmp2, bits);
    assembler->mov(stackVar(tmp1), tmp2);
    return true;
}
