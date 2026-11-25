//
// The resume check and the dispatch: "continue at task->pc" (jit/x64/sections/dispatch.cpp).
//
//      resume check:   if(thread->signal) look at it first    (the interpreter's state_check:)
//                      PA = task->pc - task->rom               (in instructions)
//      dispatch:       goto jumpTable[PA]
//
#include "../arm_generator.h"

namespace armjit {

void ArmGenerator::emit_dispatch() {
    // ---- a direct call came back with something other than JIT_OK (opcode/direct_calls.cpp)
    //      if(task->current != this function) return JIT_CONTEXT_SWITCH;   // the callee is still on the stack
    //      it is gone (an exception): look at the signal
    m.bind(callNotFinished);
    m.p_load(PA, TASK, (int32_t) offsetof(fiber, current));
    m.p_constant(PB, (uintptr_t) function);
    m.p_jump_if(PA, PB, P_NE, exitSwitch);
    load_stack_registers();

    // ---- the resume check
    m.bind(resumeCheck);
    {
        int quiet = m.label();
        m.p_load_32(PC, THREAD, (int32_t) offsetof(sharp_thread, signal));
        m.p_jump_if_zero(PC, quiet);
        save_stack_registers();
        m.call(reinterpret_cast<void*>(jit_rt_safepoint), { Arg::pointer(THREAD) });
        m.result_to_pointer(PA);
        m.p_jump_if_equals(PA, jit_exception, exception);
        m.p_jump_if_equals(PA, jit_switch, exitSwitch);
        m.p_jump_if_equals(PA, jit_kill, exitKill);
        load_stack_registers();
        m.bind(quiet);
        m.p_load(PA, TASK, (int32_t) offsetof(fiber, pc));
        m.p_load(PB, TASK, (int32_t) offsetof(fiber, rom));
        m.p_subtract(PA, PA, PB);
        m.p_shift_right(PA, 2);                 // bytes -> instruction index
    }

    // ---- dispatch: goto the instruction whose pc is in PA
    m.bind(dispatch);
    m.p_constant(PB, (uintptr_t) count);
    m.p_jump_if(PA, PB, P_HIGHER_OR_SAME, decline);     // unsigned: a negative pc is out of range too
    m.p_constant(PB, (uintptr_t) record);
    m.p_load(PB, PB, (int32_t) offsetof(jit_compiled_function, jumpTable));
    m.jump_through_table(PB, PA);
}

} // namespace armjit
