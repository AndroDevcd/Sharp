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

bool X64Compiler::emit_call(int address) {
    /*
     * VM Interpreter Code (virtual_machine.cpp):
     * CALL:
     *     prepare_method(single_arg);
     *     check_state(0) // a jit compiled callee can come back with an exception/signal pending
     */

    // The callee's frame records task->pc as the place to come back to
    storePC();

    // prepare_method(address)
    // Because the caller (us) is compiled code, prepare_method runs the callee to
    // completion before it returns here. When the callee returns normally
    // return_method() has already moved task->pc to the instruction after this CALL.
    assembler->mov(arg0.r32(), (uint32_t)address);
    callFunction(reinterpret_cast<void*>(jit_prepareMethod));

    // branch_for(0): continue wherever task->pc points now (see emitAfterCall)
    emitAfterCall();
    return true;
}
