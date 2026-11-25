//
// "An exception is pending" (jit/x64/sections/exception.cpp): ask the runtime for a handler in
// this function.
//
//      jit_rt_catch    1: there is one, task->pc is at it - continue there
//                      2: the thread is being stopped
//                      0: none here - pop our frame (jit_rt_leave) and leave with JIT_EXCEPTION
//
#include "../arm_generator.h"

namespace armjit {

void ArmGenerator::emit_exception() {
    int caught = m.label();
    m.bind(exception);
    m.call(reinterpret_cast<void*>(jit_rt_catch), { Arg::pointer(THREAD) });
    m.result_to_pointer(PA);
    m.p_jump_if_equals(PA, 1, caught);
    m.p_jump_if_equals(PA, 2, exitKill);
    m.call(reinterpret_cast<void*>(jit_rt_leave), { Arg::pointer(THREAD), Arg::constant(JIT_EXCEPTION) });
    m.jump(leave);                              // what it returned is the return code
    m.bind(caught);
    load_stack_registers();
    m.jump(resumeCheck);
}

} // namespace armjit
