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

bool X64Compiler::emit_newclass(int address) {
    /*
     * VM Interpreter Code (virtual_machine.cpp):
     * NEWCLASS:
     *     grow_stack
     *     stack_overflow_check
     *     copy_object(&push_stack_object, create_object(&vm.classes[raw_arg2]));
     *     check_state(2)
     */

    emitStackCheck(1);

    // rax = create_object(&vm.classes[address])
    // (vm.classes never moves after the program is loaded, so the address of the
    //  class is a compile time constant here)
    assembler->mov(arg0, reinterpret_cast<uint64_t>(&vm.classes[address]));
    callFunction(reinterpret_cast<void*>(jit_createObject));
    emitFastExceptionCheck();                      // does not modify rax
    assembler->mov(tmp2, x86::rax);

    // arg0 = &(++task->sp)->obj
    loadTask(tmp0);
    assembler->mov(tmp1, x86::qword_ptr(tmp0, offsetof(fiber, sp)));
    assembler->add(tmp1, (int)sizeof(stack_item));
    assembler->mov(x86::qword_ptr(tmp0, offsetof(fiber, sp)), tmp1);
    assembler->lea(arg0, stackObj(tmp1));

    // copy_object(arg0, newObject)
    assembler->mov(arg1, tmp2);
    callFunction(reinterpret_cast<void*>(jit_copyObject1));

    // check_state(2)   (two word instruction)
    emitStateCheck(2);
    return true;
}
