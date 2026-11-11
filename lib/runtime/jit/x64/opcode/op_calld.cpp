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

bool X64Compiler::emit_calld(int inRegister) {
    /*
     * VM Interpreter Code (virtual_machine.cpp):
     * CALLD:
     *     if((data = (Int)regs[single_arg]) <= 0 || data >= vm.mf.methods) {
     *         stringstream ss;
     *         ss << "invalid call to method (@" << data << ") out of range!";
     *         throw vm_exception(ss.str());
     *     }
     *     prepare_method(data);
     *     check_state(0)
     */

    storePC();

    // the range check + prepare_method((Int)regs[inRegister]) is done in C++
    assembler->mov(arg0.r32(), (uint32_t)inRegister);
    callFunction(reinterpret_cast<void*>(jit_prepareMethodDynamic));

    emitAfterCall();
    return true;
}
