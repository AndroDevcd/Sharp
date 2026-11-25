//
// The ways out of a compiled function (jit/x64/sections/exits.cpp). Each sets the return code and
// goes to `leave`, where the body returns to the door it was entered through.
//
//      exitSwitch      JIT_CONTEXT_SWITCH   not finished: the frame and task->pc stay
//      exitKill        JIT_KILL             the thread is stopping
//      decline         JIT_CONTEXT_SWITCH   task->pc is not an instruction of ours: the interpreter goes on
//
#include "../arm_generator.h"

namespace armjit {

void ArmGenerator::emit_exits() {
    m.bind(exitSwitch);
    m.return_code(JIT_CONTEXT_SWITCH);
    m.jump(leave);

    m.bind(exitKill);
    m.return_code(JIT_KILL);
    m.jump(leave);

    m.bind(decline);
    m.return_code(JIT_CONTEXT_SWITCH);

    m.bind(leave);
    m.body_return();
}

} // namespace armjit
