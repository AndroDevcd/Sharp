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

bool X64Compiler::emit_lock() {
    /*
     * VM Interpreter Code (virtual_machine.cpp):
     * LOCK:
     *     require_object_with_value(
     *         if(lock_object(task->ptr->o)) {
     *             return;
     *         }
     *     )
     *     branch
     */

    // if the lock is taken we leave the function and retry this instruction later
    storePC();

    // tmp1 = task->ptr->o (or nullptr exception)
    emitRequireObjectWithValue(tmp1);

    // mustYield = lock_object(task->ptr->o)
    assembler->mov(arg0, tmp1);
    callFunction(reinterpret_cast<void*>(jit_lockObject));
    emitFastExceptionCheck();

    // if(lock_object(...)) return;   (a context switch was requested)
    Label gotLock = assembler->newLabel();
    assembler->test(x86::al, x86::al);
    assembler->jz(gotLock);
    emitReturn(JIT_CONTEXT_SWITCH);
    assembler->bind(gotLock);
    return true;
}
