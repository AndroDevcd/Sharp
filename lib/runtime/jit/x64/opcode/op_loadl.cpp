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

bool X64Compiler::emit_loadl(int outRegister, int relFrameAddress) {
    /*
     * VM Interpreter Code (virtual_machine.cpp):
     * LOADL:
     *     regs[dual_arg1] = (task->fp + dual_arg2)->var;
     *     branch
     */

    // tmp1 = task->fp
    loadTask(tmp0);
    assembler->mov(tmp1, x86::qword_ptr(tmp0, offsetof(fiber, fp)));

    // regs[outRegister] = (tmp1 + relFrameAddress)->var
    assembler->fld(stackVar(tmp1, relFrameAddress));
    fstpRegister(outRegister);
    return true;
}
