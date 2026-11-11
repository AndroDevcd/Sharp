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

bool X64Compiler::emit_smovr_4(int relFrameAddress1, int relFrameAddress2) {
    /*
     * VM Interpreter Code (virtual_machine.cpp):
     * SMOVR_4:
     *     (task->fp + single_arg)->var = (task->fp + raw_arg2)->var;
     *     branch_for(2)
     */

    // (task->fp + relFrameAddress1)->var = (task->fp + relFrameAddress2)->var
    loadTask(tmp0);
    assembler->mov(tmp1, x86::qword_ptr(tmp0, offsetof(fiber, fp)));
    assembler->mov(tmp2, stackVar(tmp1, relFrameAddress2));
    assembler->mov(stackVar(tmp1, relFrameAddress1), tmp2);
    return true;
}
