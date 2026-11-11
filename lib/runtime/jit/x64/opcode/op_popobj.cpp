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

bool X64Compiler::emit_popobj() {
    /*
     * VM Interpreter Code (virtual_machine.cpp):
     * POPOBJ:
     *     require_object(
     *         copy_object(task->ptr, pop_stack_object.o);
     *     )
     *     branch
     */

    // require_object: task->ptr must not be null
    loadTask(tmp0);
    assembler->mov(tmp2, x86::qword_ptr(tmp0, offsetof(fiber, ptr)));
    assembler->test(tmp2, tmp2);
    assembler->jz(nullptrExceptionLabel);

    // copy_object(task->ptr, pop_stack_object.o)
    emitPopSlot(tmp1);
    assembler->mov(arg1, stackObj(tmp1));
    assembler->mov(arg0, tmp2);
    callFunction(reinterpret_cast<void*>(jit_copyObject1));
    emitFastExceptionCheck();
    return true;
}
