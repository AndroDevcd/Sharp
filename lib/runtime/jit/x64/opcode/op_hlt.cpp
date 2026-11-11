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

bool X64Compiler::emit_hlt() {
    /*
     * VM Interpreter Code (virtual_machine.cpp):
     * HLT:
     *     send_interrupt_signal(thread);
     *     return;
     */

    // send_interrupt_signal(thread); return;
    storePC();
    assembler->mov(arg0, threadPtr);
    callFunction(reinterpret_cast<void*>(send_interrupt_signal));
    emitReturn(JIT_KILL);
    return true;
}
