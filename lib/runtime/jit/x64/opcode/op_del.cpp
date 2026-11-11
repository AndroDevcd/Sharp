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

bool X64Compiler::emit_del() {
    /*
     * VM Interpreter Code (virtual_machine.cpp):
     * DEL:
     *     copy_object(task->ptr, (sharp_object*) nullptr);
     *     branch
     */

    // The interpreter does not check task->ptr here (a null ptr would crash the
    // vm), compiled code raises a nullptr exception instead.
    loadTask(tmp0);
    assembler->mov(arg0, x86::qword_ptr(tmp0, offsetof(fiber, ptr)));
    assembler->test(arg0, arg0);
    assembler->jz(nullptrExceptionLabel);

    // copy_object(task->ptr, (sharp_object*) nullptr)
    assembler->xor_(arg1.r32(), arg1.r32());
    callFunction(reinterpret_cast<void*>(jit_copyObject1));
    emitFastExceptionCheck();
    return true;
}
