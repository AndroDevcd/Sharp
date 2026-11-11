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

bool X64Compiler::emit_checknull(int outRegister) {
    /*
     * VM Interpreter Code (virtual_machine.cpp):
     * CHECKNULL:
     *     regs[single_arg] = task->ptr == nullptr || task->ptr->o == nullptr;
     *     branch
     */

    // regs[outRegister] = task->ptr == nullptr || task->ptr->o == nullptr
    Label done = assembler->newLabel();
    assembler->mov(x86::eax, 1);                            // assume null
    loadTask(tmp1);
    assembler->mov(tmp1, x86::qword_ptr(tmp1, offsetof(fiber, ptr)));
    assembler->test(tmp1, tmp1);
    assembler->jz(done);
    assembler->cmp(x86::qword_ptr(tmp1, offsetof(object, o)), 0);
    assembler->je(done);
    assembler->xor_(x86::eax, x86::eax);                    // both are there
    assembler->bind(done);

    emitStoreBool(outRegister);
    return true;
}
