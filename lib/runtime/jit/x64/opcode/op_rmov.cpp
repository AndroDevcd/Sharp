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

bool X64Compiler::emit_rmov(int indexRegister, int inRegister) {
    /*
     * VM Interpreter Code (virtual_machine.cpp):
     * RMOV:
     *     require_numeric_object_with_value(
     *         #ifdef SAFE_EXECUTION
     *         if(task->ptr->o->type > type_var) {
     *             vm_exception err(vm.ill_state_except, "numeric access on data structure");
     *             enable_exception_flag(thread, true);
     *             goto catch_exception;
     *         }
     *         #endif
     *         task->ptr->o->HEAD[(Int)regs[dual_arg1]] =
     *                 regs[dual_arg2];
     *     )
     *     branch
     */

    // tmp1 = task->ptr->o->HEAD   (throws nullptr exception if null or not a numeric object)
    emitRequireNumericObjectWithValue(tmp1);
    assembler->mov(tmp1, x86::qword_ptr(tmp1, offsetof(sharp_object, HEAD)));

    // HEAD[(Int)regs[indexRegister]] = regs[inRegister]
    emitRegisterToInt(indexRegister, tmp0);
    fldRegister(inRegister);
    assembler->fstp(headElement(tmp1, tmp0));
    return true;
}
