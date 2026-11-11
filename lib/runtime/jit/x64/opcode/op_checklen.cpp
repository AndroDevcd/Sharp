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

bool X64Compiler::emit_checklen(int inRegister) {
    /*
     * VM Interpreter Code (virtual_machine.cpp):
     * CHECKLEN:
     *     require_object_with_value(
     *             if(regs[single_arg] < task->ptr->o->size && regs[single_arg] >= 0) {
     *                 branch
     *             }
     *             else {
     *                 stringstream ss;
     *                 ss << "Access to Object at: " << regs[single_arg] << " size is " << task->ptr->o->size;
     *                 throw vm_exception(vm.bounds_except, ss.str());
     *             }
     *     )
     */

    // tmp1 = task->ptr->o   (throws nullptr exception if task->ptr or ->o is null)
    emitRequireObjectWithValue(tmp1);

    // Fast path: if((uInt)(Int)regs[inRegister] < task->ptr->o->size) continue;
    // One unsigned compare covers both "index < size" and "index >= 0" because a
    // negative index becomes a huge unsigned number.
    Label inBounds = assembler->newLabel();
    emitRegisterToInt(inRegister, tmp0);
    assembler->mov(tmp2.r32(), x86::dword_ptr(tmp1, offsetof(sharp_object, size)));   // zero extends
    assembler->cmp(tmp0, tmp2);
    assembler->jb(inBounds);

    // Slow path: let C++ redo the exact check and build the "Access to Object
    // at: ..." bounds exception.
    storePC();
    assembler->mov(arg0.r32(), (uint32_t)inRegister);
    callFunction(reinterpret_cast<void*>(jit_checkLen));
    emitFastExceptionCheck();

    assembler->bind(inBounds);
    return true;
}
