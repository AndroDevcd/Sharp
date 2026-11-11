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

bool X64Compiler::emit_varcast(int varType, bool isArray) {
    /*
     * VM Interpreter Code (virtual_machine.cpp):
     * VARCAST:
     *     require_object_with_value(
     *         cast_numeric_array(task->ptr, dual_arg1);
     *     )
     *     branch
     */

    // require_object_with_value( ... )
    emitRequireObjectWithValue(tmp1);

    // the wrapper may allocate, throw or print: the vm state has to be up to date
    storePC();

    assembler->mov(arg0.r32(), (uint32_t)varType);
    callFunction(reinterpret_cast<void*>(jit_varCast));
    emitFastExceptionCheck();

    return true;
}
