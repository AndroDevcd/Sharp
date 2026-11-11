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

bool X64Compiler::emit_itest(int outRegister) {
    /*
     * VM Interpreter Code (virtual_machine.cpp):
     * ITEST:
     *     data = (Int)pop_stack_object.o;
     *     regs[single_arg] = (sharp_object*)data == pop_stack_object.o;
     *     branch
     */

    // the wrapper may allocate, throw or print: the vm state has to be up to date
    storePC();

    assembler->mov(arg0.r32(), (uint32_t)outRegister);
    callFunction(reinterpret_cast<void*>(jit_itest));
    emitFastExceptionCheck();

    return true;
}
