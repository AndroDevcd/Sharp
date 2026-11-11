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

bool X64Compiler::emit_movsl(int relStackAddress) {
    /*
     * VM Interpreter Code (virtual_machine.cpp):
     * MOVSL:
     *     task->ptr = &((task->sp+single_arg)->obj);
     *     branch
     */

    // task->ptr = &(task->sp + relStackAddress)->obj
    loadTask(tmp0);
    assembler->mov(tmp1, x86::qword_ptr(tmp0, offsetof(fiber, sp)));
    assembler->lea(tmp1, stackObj(tmp1, relStackAddress));
    assembler->mov(x86::qword_ptr(tmp0, offsetof(fiber, ptr)), tmp1);
    return true;
}
