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

bool X64Compiler::emit_throw() {
    /*
     * VM Interpreter Code (virtual_machine.cpp):
     * THROW:
     *     copy_object(&task->exceptionObject, pop_stack_object.o);
     *     enable_exception_flag(thread, true);
     *     check_state(0)
     */

    // copy_object(&task->exceptionObject, pop_stack_object.o)
    emitPopSlot(tmp1);
    assembler->mov(arg1, stackObj(tmp1));
    loadTask(tmp0);
    assembler->lea(arg0, x86::ptr(tmp0, offsetof(fiber, exceptionObject)));
    callFunction(reinterpret_cast<void*>(jit_copyObject1));

    // enable_exception_flag(thread, true)
    assembler->mov(arg0, threadPtr);
    assembler->mov(arg1.r32(), 1);
    callFunction(reinterpret_cast<void*>(jit_enableExceptionFlag));

    // check_state(0) with the exception flag set always ends in catch_exception
    assembler->jmp(catchExceptionLabel);
    return true;
}
