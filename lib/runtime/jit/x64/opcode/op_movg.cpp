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

bool X64Compiler::emit_movg(int address) {
    /*
     * VM Interpreter Code (virtual_machine.cpp):
     * MOVG:
     *     task->ptr = vm.staticHeap + single_arg;
     *     branch
     */

    // task->ptr = vm.staticHeap + address       (the static heap never moves, so
    // the address of the slot is a constant while compiling)
    loadTask(tmp0);
    assembler->mov(tmp1, reinterpret_cast<uint64_t>(vm.staticHeap + address));
    assembler->mov(x86::qword_ptr(tmp0, offsetof(fiber, ptr)), tmp1);
    return true;
}
