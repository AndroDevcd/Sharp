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

bool X64Compiler::emit_sizeof(int outRegister) {
    /*
     * VM Interpreter Code (virtual_machine.cpp):
     * SIZEOF:
     *     regs[single_arg]= ( task->ptr!=NULL && task->ptr->o != NULL) ? task->ptr->o->size : 0;
     *     branch
     */

    // regs[outRegister] = (task->ptr != NULL && task->ptr->o != NULL) ? task->ptr->o->size : 0
    Label done = assembler->newLabel();
    assembler->xor_(x86::eax, x86::eax);                    // rax = 0
    loadTask(tmp1);
    assembler->mov(tmp1, x86::qword_ptr(tmp1, offsetof(fiber, ptr)));
    assembler->test(tmp1, tmp1);
    assembler->jz(done);
    assembler->mov(tmp1, x86::qword_ptr(tmp1, offsetof(object, o)));
    assembler->test(tmp1, tmp1);
    assembler->jz(done);
    assembler->mov(x86::eax, x86::dword_ptr(tmp1, offsetof(sharp_object, size)));   // size is a uint32_t
    assembler->bind(done);

    assembler->mov(scratch(8), tmp0);
    assembler->fild(scratch(8));
    fstpRegister(outRegister);
    return true;
}
