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

bool X64Compiler::emit_test(int leftRegister, int rightRegister) {
    /*
     * VM Interpreter Code (virtual_machine.cpp):
     * TEST:
     *     regs[BMR] = regs[dual_arg1] == regs[dual_arg2];
     *     branch
     */

    // regs[BMR] = regs[leftRegister] == regs[rightRegister]
    emitCompare(leftRegister, rightRegister, BMR, JIT_CMP_EQ);
    return true;
}
