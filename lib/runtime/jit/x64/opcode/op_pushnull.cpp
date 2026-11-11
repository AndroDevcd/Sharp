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

bool X64Compiler::emit_pushnull() {
    /*
     * VM Interpreter Code (virtual_machine.cpp):
     * PUSHNULL:
     *     grow_stack
     *     stack_overflow_check
     *     copy_object(&push_stack_object, (sharp_object*) nullptr);
     *     branch
     */

    emitStackCheck(1);

    // copy_object(&(++task->sp)->obj, (sharp_object*) nullptr)
    emitPushSlot(tmp1);
    assembler->lea(arg0, stackObj(tmp1));
    assembler->xor_(arg1.r32(), arg1.r32());
    callFunction(reinterpret_cast<void*>(jit_copyObject1));
    emitFastExceptionCheck();
    return true;
}
