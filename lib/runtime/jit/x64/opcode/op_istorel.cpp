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

bool X64Compiler::emit_istorel(int relFrameAddress, int integerValue) {
    /*
     * VM Interpreter Code (virtual_machine.cpp):
     * ISTOREL:
     *     (task->fp + single_arg)->var = raw_arg2;
     *     branch_for(2)
     */

    // (task->fp + relFrameAddress)->var = (double)integerValue
    loadTask(tmp0);
    assembler->mov(tmp1, x86::qword_ptr(tmp0, offsetof(fiber, fp)));
    double number = (double)integerValue;
    uint64_t bits;
    memcpy(&bits, &number, sizeof(bits));
    assembler->mov(tmp2, bits);
    assembler->mov(stackVar(tmp1, relFrameAddress), tmp2);
    return true;
}
