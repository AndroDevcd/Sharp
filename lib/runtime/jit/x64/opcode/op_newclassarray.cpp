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

bool X64Compiler::emit_newclassarray(int inRegister, int classAddress) {
    /*
     * VM Interpreter Code (virtual_machine.cpp):
     * NEWCLASSARRAY:
     *     grow_stack
     *     stack_overflow_check
     *     copy_object(
     *             &push_stack_object,
     *             create_object(vm.classes + raw_arg2, (Int)regs[single_arg])
     *     );
     *     check_state(2)
     */

    emitStackCheck(1);

    // the wrapper may allocate, throw or print: the vm state has to be up to date
    storePC();

    assembler->mov(arg0.r32(), (uint32_t)inRegister);
    assembler->mov(arg1.r32(), (uint32_t)classAddress);
    callFunction(reinterpret_cast<void*>(jit_newClassArray));
    emitFastExceptionCheck();

    // check_state(2)   (two word instruction)
    emitBranchTo(currentPC + 2);
    return true;
}
