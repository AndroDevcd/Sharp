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

bool X64Compiler::emit_andl(int inRegister, int relStackAddress) {
    /*
     * VM Interpreter Code (virtual_machine.cpp):
     * ANDL:
     *     (task->fp + dual_arg2)->var = (Int)(task->fp + dual_arg2)->var & (Int)regs[dual_arg1];
     *     branch
     */

    // (task->fp + relStackAddress)->var = (Int)(task->fp + relStackAddress)->var & (Int)regs[inRegister]
    emitRegisterToInt(inRegister, tmp2);

    loadTask(tmp0);
    assembler->mov(tmp1, x86::qword_ptr(tmp0, offsetof(fiber, fp)));
    assembler->fld(stackVar(tmp1, relStackAddress));
    assembler->fisttp(scratch(8));                 // (Int)local
    assembler->mov(tmp0, scratch(8));

    assembler->and_(tmp0, tmp2);
    assembler->mov(scratch(8), tmp0);
    assembler->fild(scratch(8));
    assembler->fstp(stackVar(tmp1, relStackAddress));
    return true;
}
