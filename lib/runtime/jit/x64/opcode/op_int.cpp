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

bool X64Compiler::emit_int(int flag) {
    /*
     * VM Interpreter Code (virtual_machine.cpp):
     * _INT:
     *     {
     *         TELEMETRY_SCOPE(tm_mode_interrupt)
     *         TELEMETRY_COUNT(tm_interrupts)
     *         exec_interrupt(single_arg);
     *     }
     *     if(vm.state == VM_TERMINATED) {
     *         send_interrupt_signal(thread);
     *         return;
     *     }
     *     check_state(1)
     */

    // interrupts can do almost anything (io, start threads, delay the fiber...)
    // so the VM state has to be fully up to date first
    storePC();

    // terminated = jit_execInterrupt(flag)   [exec_interrupt + the VM_TERMINATED check]
    assembler->mov(arg0.r32(), (uint32_t)flag);
    callFunction(reinterpret_cast<void*>(jit_execInterrupt));

    Label alive = assembler->newLabel();
    assembler->test(x86::al, x86::al);
    assembler->jz(alive);
    emitReturn(JIT_KILL);                  // the vm is shutting down: just leave
    assembler->bind(alive);

    // an exception thrown by the interrupt belongs to this instruction
    emitFastExceptionCheck();

    // check_state(1):  task->pc += 1; goto state_check;
    loadTask(tmp0);
    assembler->add(x86::qword_ptr(tmp0, offsetof(fiber, pc)), (int)sizeof(opcode_instr));
    emitAfterCall();
    return true;
}
