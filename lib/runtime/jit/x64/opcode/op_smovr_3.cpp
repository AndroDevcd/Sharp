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

bool X64Compiler::emit_smovr_3(int relFrameAddress) {
    /*
     * VM Interpreter Code (virtual_machine.cpp):
     * SMOVR_3:
     *     copy_object(&(task->fp + single_arg)->obj, task->ptr);
     *     branch
     */

    // copy_object(&(task->fp + relFrameAddress)->obj, task->ptr)
    loadTask(tmp0);
    assembler->mov(arg1, x86::qword_ptr(tmp0, offsetof(fiber, ptr)));
    assembler->mov(tmp1, x86::qword_ptr(tmp0, offsetof(fiber, fp)));
    assembler->lea(arg0, stackObj(tmp1, relFrameAddress));
    callFunction(reinterpret_cast<void*>(jit_copyObject2));
    emitFastExceptionCheck();
    return true;
}
