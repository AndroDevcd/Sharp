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

bool X64Compiler::emit_mod(int outRegister, int leftRegister, int rightRegister) {
    /*
     * VM Interpreter Code (virtual_machine.cpp):
     * MOD:
     *     regs[triple_arg3] = (Int)regs[triple_arg1] % (Int)regs[triple_arg2];
     *     branch
     */

    // the wrapper may allocate, throw or print: the vm state has to be up to date
    storePC();

    assembler->mov(arg0.r32(), (uint32_t)outRegister);
    assembler->mov(arg1.r32(), (uint32_t)leftRegister);
    assembler->mov(arg2.r32(), (uint32_t)rightRegister);
    callFunction(reinterpret_cast<void*>(jit_mod));
    emitFastExceptionCheck();

    return true;
}
