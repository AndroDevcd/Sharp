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

bool X64Compiler::emit_invoke_delegate(int address, int argCount, bool isFunctionStatic) {
    /*
     * VM Interpreter Code (virtual_machine.cpp):
     * INVOKE_DELEGATE:
     *     invoke_delegate(single_arg, dual_raw_arg2, dual_raw_arg1);
     *     check_state(0)
     */

    storePC();

    // invoke_delegate(address, argCount, isFunctionStatic)
    // It advances task->pc itself (past both words of this instruction) and, for
    // a non-static delegate, runs the resolved function through prepare_method.
    assembler->mov(arg0.r32(), (uint32_t)address);
    assembler->mov(arg1.r32(), (uint32_t)argCount);
    assembler->mov(arg2.r32(), isFunctionStatic ? 1 : 0);
    callFunction(reinterpret_cast<void*>(jit_invokeDelegate));

    // check_state(0)
    emitAfterCall();
    return true;
}
