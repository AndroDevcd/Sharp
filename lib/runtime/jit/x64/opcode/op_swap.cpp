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

bool X64Compiler::emit_swap() {
    /*
     * VM Interpreter Code (virtual_machine.cpp):
     * SWAP:
     *     if((task->sp-task->stack) >= 2) {
     *         auto swappedObj = task->sp->obj.o;
     *         (task->sp)->obj.o = (task->sp-1)->obj.o;
     *         (task->sp-1)->obj.o = swappedObj;
     *         double swappedVar = task->sp->var;
     *         (task->sp)->var = (task->sp-1)->var;
     *         (task->sp-1)->var = swappedVar;
     *     } else {
     *         stringstream ss;
     *         ss << "Illegal stack swap, not enough data";
     *         throw vm_exception(vm.thread_stack_except, ss.str());
     *     }
     *     branch
     */

    // the wrapper may allocate, throw or print: the vm state has to be up to date
    storePC();

    callFunction(reinterpret_cast<void*>(jit_swap));
    emitFastExceptionCheck();

    return true;
}
