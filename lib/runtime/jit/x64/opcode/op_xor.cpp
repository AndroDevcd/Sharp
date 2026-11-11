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

bool X64Compiler::emit_xor(int leftRegister, int rightRegister) {
    /*
     * VM Interpreter Code (virtual_machine.cpp):
     * XOR:
     *     regs[BMR] = (Int)regs[dual_arg1] ^ (Int)regs[dual_arg2];
     *     branch
     */

    // regs[BMR] = (Int)regs[leftRegister] ^ (Int)regs[rightRegister]
    emitRegisterToInt(leftRegister, tmp1);
    emitRegisterToInt(rightRegister, tmp0);
    assembler->xor_(tmp1, tmp0);
    assembler->mov(scratch(8), tmp1);
    assembler->fild(scratch(8));
    fstpRegister(BMR);
    return true;
}
