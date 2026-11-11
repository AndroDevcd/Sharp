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

bool X64Compiler::emit_imod(int outRegister, int value) {
    /*
     * VM Interpreter Code (virtual_machine.cpp):
     * IMOD:
     *     regs[single_arg] = (Int)regs[single_arg] % raw_arg2;
     *     branch_for(2)
     */

    // the wrapper may allocate, throw or print: the vm state has to be up to date
    storePC();

    assembler->mov(arg0.r32(), (uint32_t)outRegister);
    assembler->mov(arg1.r32(), (uint32_t)value);
    callFunction(reinterpret_cast<void*>(jit_imod));
    emitFastExceptionCheck();

    return true;
}
