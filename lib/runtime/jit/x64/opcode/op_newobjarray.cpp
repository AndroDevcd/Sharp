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

bool X64Compiler::emit_newobjarray(int inRegister) {
    /*
     * VM Interpreter Code (virtual_machine.cpp):
     * NEWOBJARRAY:
     *     grow_stack
     *     stack_overflow_check
     *     copy_object(&push_stack_object, create_object(regs[single_arg]));
     *     check_state(1)
     */

    emitStackCheck(1);

    // the wrapper may allocate, throw or print: the vm state has to be up to date
    storePC();

    assembler->mov(arg0.r32(), (uint32_t)inRegister);
    callFunction(reinterpret_cast<void*>(jit_newObjArray));
    emitFastExceptionCheck();

    // check_state(1)
    emitBranchTo(currentPC + 1);
    return true;
}
