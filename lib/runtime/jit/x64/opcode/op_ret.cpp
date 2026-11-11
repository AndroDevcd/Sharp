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

bool X64Compiler::emit_ret(int errState) {
    /*
     * VM Interpreter Code (virtual_machine.cpp):
     * RET:
     *     data = single_arg;
     *     if(data == ERR_STATE && task->exceptionObject.o == nullptr) {
     *         copy_object(
     *                 &task->exceptionObject,
     *                 pop_stack_object.o
     *         );
     *     }
     *     if(return_method()) {
     *         if(data == ERR_STATE) {
     *             // Leave task->pc on the CALL in the caller: that is the pc
     *             // catch_exception() uses to find the enclosing try block.
     *             enable_exception_flag(thread, true);
     *         } else {
     *             task->pc++;
     *         }
     *         return;
     *     }
     *     if(data == ERR_STATE) {
     *         enable_exception_flag(thread, true);
     *         goto catch_exception;
     *     }
     *     check_state(1)
     */

    if(errState == ERR_STATE) {
        // if(task->exceptionObject.o == nullptr)
        //     copy_object(&task->exceptionObject, pop_stack_object.o);
        Label hasExceptionObject = assembler->newLabel();
        loadTask(tmp0);
        assembler->cmp(x86::qword_ptr(tmp0, offsetof(fiber, exceptionObject) + offsetof(object, o)), 0);
        assembler->jne(hasExceptionObject);

        // arg1 = (task->sp--)->obj.o
        assembler->mov(tmp1, x86::qword_ptr(tmp0, offsetof(fiber, sp)));
        assembler->mov(arg1, stackObj(tmp1));
        assembler->sub(tmp1, (int)sizeof(stack_item));
        assembler->mov(x86::qword_ptr(tmp0, offsetof(fiber, sp)), tmp1);

        // arg0 = &task->exceptionObject
        assembler->lea(arg0, x86::ptr(tmp0, offsetof(fiber, exceptionObject)));
        callFunction(reinterpret_cast<void*>(jit_copyObject1));

        assembler->bind(hasExceptionObject);
    }

    // return_method() and the pc update happen in the shared return section
    emitReturn(errState == ERR_STATE ? JIT_EXCEPTION : JIT_OK);
    return true;
}
