//
// Entry: how a compiled function is entered. The doors of x86-64 (jit/x64/sections/entry.cpp):
//
//      THE C++ DOOR    int function(sharp_thread *thread, jit_compiled_function *fun, vm_cell *registers)
//          what the VM calls. It saves what C++ wants back, loads thread, task, regs, fp, sp, ptr,
//          and calls the body through one of the two doors below:
//
//              if(task->rebuilding) run the frames above us first      (a fiber is being continued)
//              if(task->pc == task->rom and no signal is pending)      the direct door
//              else                                                    the resume door
//
//      THE DIRECT DOOR     "from the first instruction": the instructions follow it. What other
//                          compiled code calls (opcode/direct_calls.cpp); the C++ door has a twin of
//                          it, which notes in the body's frame that C++ is the caller
//      THE RESUME DOOR     "continue at task->pc": leads to the resume check (sections/dispatch.cpp)
//
// FRAME_DIRECT in the body's frame says who called: RET leaves the short way for compiled code,
// which puts its own state back, and the whole way (jit_rt_leave) for C++.
//
// The body returns to the C++ door with the return code set (sections/exits.cpp).
//
#include "../arm_generator.h"

namespace armjit {

void ArmGenerator::emit_entry() {
    int notRebuilding = m.label(), inTheMiddle = m.label();
    int bodyReturned = m.label();

    // ---------------------------------------------------------------- the C++ door
    m.door_enter();
    m.p_load(TASK, THREAD, (int32_t) offsetof(sharp_thread, task));

    // if(task->rebuilding) { run the frames above this one; then see what happened }
    m.p_load_8(PA, TASK, (int32_t) offsetof(fiber, rebuilding));
    m.p_jump_if_zero(PA, notRebuilding);
    m.p_constant(PA, (uintptr_t) record);
    m.call(reinterpret_cast<void*>(jit_rt_rebuild_above), { Arg::pointer(PA) });
    // The frames above us have run. If the function on top is still not us, they did not finish
    // (another switch): unwind again, nothing was loaded.
    m.p_load(PA, TASK, (int32_t) offsetof(fiber, current));
    m.p_constant(PB, (uintptr_t) function);
    m.return_code(JIT_CONTEXT_SWITCH);
    m.p_jump_if(PA, PB, P_NE, bodyReturned);
    load_stack_registers();
    m.call_label(resumeEntry);
    m.jump(bodyReturned);

    m.bind(notRebuilding);
    load_stack_registers();
    m.p_load(PA, TASK, (int32_t) offsetof(fiber, pc));
    m.p_load(PB, TASK, (int32_t) offsetof(fiber, rom));
    m.p_jump_if(PA, PB, P_NE, inTheMiddle);
    // pc at the first instruction is not always a fresh call: a function whose first instruction
    // is a CALL is continued THERE when the callee threw. With a signal pending take the resume
    // door, which looks at it before anything runs.
    jump_if_signal(inTheMiddle);
    m.call_label(doorEntry);
    m.jump(bodyReturned);

    m.bind(inTheMiddle);                    // e.g. a hot loop taken over from the interpreter
    m.call_label(resumeEntry);

    m.bind(bodyReturned);
    m.door_leave();

    // ---------------------------------------------------------------- the resume door
    m.bind(resumeEntry);
    m.body_enter();
    m.p_constant(PA, 0);
    m.frame_store(FRAME_DIRECT, PA);        // C++ is the caller: return the whole way
    m.jump(resumeCheck);

    // ---------------------------------------------------------------- the direct door, for the C++ door above
    m.bind(doorEntry);
    m.body_enter();
    m.p_constant(PA, 0);
    m.frame_store(FRAME_DIRECT, PA);
    m.jump(labels[0]);

    // ---------------------------------------------------------------- the direct door
    // (must stay last: the first instruction of the function follows)
    m.bind(directEntry);
    m.body_enter();
    m.p_constant(PA, 1);
    m.frame_store(FRAME_DIRECT, PA);        // compiled code is the caller: it puts its own state back
    m.stack_numbers_changed();              // (the caller had its own locals in the registers of ours)
}

} // namespace armjit
