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

bool X64Compiler::emit_ulock() {
    /*
     * VM Interpreter Code (virtual_machine.cpp):
     * ULOCK:
     *     require_object_with_value(
     *         unlock_object(task->ptr->o);
     *     )
     *     branch
     */

    emitRequireObjectWithValue(tmp1);

    // unlock_object(task->ptr->o)
    assembler->mov(arg0, tmp1);
    callFunction(reinterpret_cast<void*>(jit_unlockObject));
    emitFastExceptionCheck();
    return true;
}
