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

bool X64Compiler::emit_shr(int outRegister, int leftRegister, int rightRegister) {
    /*
     * VM Interpreter Code (virtual_machine.cpp):
     * SHR:
     *     regs[triple_arg3] = (Int)regs[triple_arg1] >> (Int)regs[triple_arg2];
     *     branch
     */

    // regs[outRegister] = (Int)regs[leftRegister] >> (Int)regs[rightRegister]
    // x86 takes a variable shift count in cl. rcx is not one of our long-lived
    // registers so it is free to use here.
    emitRegisterToInt(leftRegister, tmp1);
    emitRegisterToInt(rightRegister, x86::rcx);
    assembler->sar(tmp1, x86::cl);
    assembler->mov(scratch(8), tmp1);
    assembler->fild(scratch(8));
    fstpRegister(outRegister);
    return true;
}
