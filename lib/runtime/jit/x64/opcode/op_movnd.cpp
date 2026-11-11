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

bool X64Compiler::emit_movnd(int inRegister) {
    /*
     * VM Interpreter Code (virtual_machine.cpp):
     * MOVND:
     *     require_object_with_value(
     *         #ifdef SAFE_EXECUTION
     *          if(task->ptr->o->type <= type_var) {
     *              vm_exception err(vm.ill_state_except, "movn on number");
     *              enable_exception_flag(thread, true);
     *              goto catch_exception;
     *          }
     *         #endif
     *         task->ptr = task->ptr->o->node + (Int)regs[single_arg];
     *     )
     *     branch
     */

    // tmp1 = task->ptr->o->node   (throws nullptr exception if task->ptr or ->o is null)
    emitRequireObjectWithValue(tmp1);
    assembler->mov(tmp1, x86::qword_ptr(tmp1, offsetof(sharp_object, node)));

    // task->ptr = node + (Int)regs[inRegister]
    emitRegisterToInt(inRegister, tmp0);
    assembler->lea(tmp1, x86::ptr(tmp1, tmp0, 3));          // 8 bytes per object slot
    loadTask(tmp0);
    assembler->mov(x86::qword_ptr(tmp0, offsetof(fiber, ptr)), tmp1);
    return true;
}
