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

bool X64Compiler::emit_ldc(int outRegister, int address) {
    /*
     * VM Interpreter Code (virtual_machine.cpp):
     * LDC: // tested
     *     regs[dual_arg1]=vm.constants[dual_arg2];
     *     branch
     */

    // regs[outRegister] = vm.constants[address]     (constants is a double[] that never moves)
    assembler->mov(tmp1, reinterpret_cast<uint64_t>(&vm.constants[address]));
    assembler->fld(x86::qword_ptr(tmp1));
    fstpRegister(outRegister);
    return true;
}
