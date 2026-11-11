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

bool X64Compiler::emit_iload(int outRegister) {
    /*
     * VM Interpreter Code (virtual_machine.cpp):
     * ILOAD:
     *     require_numeric_object_with_value(
     *         #ifdef SAFE_EXECUTION
     *         if(task->ptr->o->type > type_var) {
     *             vm_exception err(vm.ill_state_except, "numeric access on data structure");
     *             enable_exception_flag(thread, true);
     *             goto catch_exception;
     *         }
     *         #endif
     *         regs[single_arg] = task->ptr->o->HEAD[0];
     *     )
     *     branch
     */

    // tmp1 = task->ptr->o->HEAD   (throws nullptr exception if null or not a numeric object)
    emitRequireNumericObjectWithValue(tmp1);
    assembler->mov(tmp1, x86::qword_ptr(tmp1, offsetof(sharp_object, HEAD)));

    // regs[outRegister] = HEAD[0]
    assembler->xor_(x86::eax, x86::eax);
    assembler->fld(headElement(tmp1, tmp0));
    fstpRegister(outRegister);
    return true;
}
