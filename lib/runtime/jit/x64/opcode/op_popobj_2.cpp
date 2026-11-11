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

bool X64Compiler::emit_popobj_2() {
    /*
     * VM Interpreter Code (virtual_machine.cpp):
     * POPOBJ_2:
     *     task->ptr = &pop_stack_object;
     *     branch
     */

    // task->ptr = &(task->sp--)->obj
    emitPopSlot(tmp1);
    assembler->lea(tmp1, stackObj(tmp1));
    loadTask(tmp0);
    assembler->mov(x86::qword_ptr(tmp0, offsetof(fiber, ptr)), tmp1);
    return true;
}
