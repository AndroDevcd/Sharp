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

bool X64Compiler::emit_dup() {
    /*
     * VM Interpreter Code (virtual_machine.cpp):
     * DUP:
     *     data = (Int)&task->sp->obj;
     *     copy_object(&push_stack_object, (object*)data);
     *     branch
     */

    // arg1 = &task->sp->obj                               (source object*)
    loadTask(tmp0);
    assembler->mov(tmp1, x86::qword_ptr(tmp0, offsetof(fiber, sp)));
    assembler->lea(arg1, stackObj(tmp1));

    // arg0 = &(++task->sp)->obj                           (destination object*)
    assembler->add(tmp1, (int)sizeof(stack_item));
    assembler->mov(x86::qword_ptr(tmp0, offsetof(fiber, sp)), tmp1);
    assembler->lea(arg0, stackObj(tmp1));

    // copy_object(object *to, object *from)
    callFunction(reinterpret_cast<void*>(jit_copyObject2));
    emitFastExceptionCheck();
    return true;
}
