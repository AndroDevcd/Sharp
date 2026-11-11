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

bool X64Compiler::emit_newarray(int inRegister, int ntype) {
    /*
     * VM Interpreter Code (virtual_machine.cpp):
     * NEWARRAY:
     *     grow_stack
     *     stack_overflow_check
     *     copy_object(
     *         &push_stack_object,
     *         create_object(regs[dual_arg1], (data_type)dual_arg2)
     *     );
     *     check_state(1)
     */

    emitStackCheck(1);

    // the wrapper may allocate, throw or print: the vm state has to be up to date
    storePC();

    assembler->mov(arg0.r32(), (uint32_t)inRegister);
    assembler->mov(arg1.r32(), (uint32_t)ntype);
    callFunction(reinterpret_cast<void*>(jit_newArray));
    emitFastExceptionCheck();

    // check_state(1)
    emitBranchTo(currentPC + 1);
    return true;
}
