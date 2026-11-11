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

bool X64Compiler::emit_idivl(int value, int relFrameAddress) {
    /*
     * VM Interpreter Code (virtual_machine.cpp):
     * IDIVL:
     *     (task->fp + single_arg)->var /= raw_arg2;
     *     branch_for(2)
     */

    // (task->fp + relFrameAddress)->var /= value      (plain double arithmetic)
    loadTask(tmp0);
    assembler->mov(tmp1, x86::qword_ptr(tmp0, offsetof(fiber, fp)));
    assembler->movsd(x86::xmm0, stackVar(tmp1, relFrameAddress));
    emitLoadDoubleConstant(x86::xmm1, (double)value);
    assembler->divsd(x86::xmm0, x86::xmm1);
    assembler->movsd(stackVar(tmp1, relFrameAddress), x86::xmm0);
    return true;
}
