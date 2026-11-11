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

bool X64Compiler::emit_pushobj() {
    /*
     * VM Interpreter Code (virtual_machine.cpp):
     * PUSHOBJ:
     *     grow_stack
     *     stack_overflow_check
     *     copy_object(&push_stack_object, task->ptr);
     *     branch
     */

    emitStackCheck(1);

    // arg1 = task->ptr                                    (source object*, may be null)
    loadTask(tmp0);
    assembler->mov(arg1, x86::qword_ptr(tmp0, offsetof(fiber, ptr)));

    // arg0 = &(++task->sp)->obj                           (destination object*)
    emitPushSlot(tmp1);
    assembler->lea(arg0, stackObj(tmp1));

    // copy_object(object *to, object *from)
    callFunction(reinterpret_cast<void*>(jit_copyObject2));
    emitFastExceptionCheck();
    return true;
}
