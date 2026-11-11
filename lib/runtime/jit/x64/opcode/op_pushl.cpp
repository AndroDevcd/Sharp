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

bool X64Compiler::emit_pushl(int relFrameAddress) {
    /*
     * VM Interpreter Code (virtual_machine.cpp):
     * PUSHL:
     *     grow_stack
     *     stack_overflow_check
     *     copy_object(&push_stack_object, (task->fp + single_arg)->obj.o);
     *     branch
     */

    emitStackCheck(1);

    // arg1 = (task->fp + relFrameAddress)->obj.o          (source sharp_object*)
    loadTask(tmp0);
    assembler->mov(tmp1, x86::qword_ptr(tmp0, offsetof(fiber, fp)));
    assembler->mov(arg1, stackObj(tmp1, relFrameAddress));

    // arg0 = &(++task->sp)->obj                           (destination object*)
    assembler->mov(tmp1, x86::qword_ptr(tmp0, offsetof(fiber, sp)));
    assembler->add(tmp1, (int)sizeof(stack_item));
    assembler->mov(x86::qword_ptr(tmp0, offsetof(fiber, sp)), tmp1);
    assembler->lea(arg0, stackObj(tmp1));

    // copy_object(arg0, arg1)
    callFunction(reinterpret_cast<void*>(jit_copyObject1));
    emitFastExceptionCheck();
    return true;
}
