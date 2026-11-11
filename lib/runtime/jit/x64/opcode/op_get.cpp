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

bool X64Compiler::emit_get(int outRegister) {
    /*
     * VM Interpreter Code (virtual_machine.cpp):
     * GET:
     *     regs[single_arg] = (regs[CMT] != 0) ? getche() : getch();
     *     branch
     */

    // the wrapper may allocate, throw or print: the vm state has to be up to date
    storePC();

    assembler->mov(arg0.r32(), (uint32_t)outRegister);
    callFunction(reinterpret_cast<void*>(jit_get));
    emitFastExceptionCheck();

    return true;
}
