//
// The exits and the epilogue: how the body of a compiled function hands control
// back to the door it was entered through (sections/entry.cpp).
//
//      exitSwitch      not finished - the fiber gives up the thread, or a callee
//                      did. The VM frame and task->pc stay; the next call of the
//                      function continues at task->pc.        -> JIT_CONTEXT_SWITCH
//      exitKill        the thread is stopping.                -> JIT_KILL
//      decline         asked to continue at a pc that is not an instruction:
//                      change nothing, the interpreter carries on from task->pc.
//      leave           memory mode, the return code is in eax: load the homes
//      epilogue        register mode, the return code is in eax: return
//
// THE RULE: the body always returns in REGISTER MODE. A direct caller keeps
// running with the VM registers where they are; the C++ door writes them to the
// register array. That is why the memory mode exits go through `leave`.
//
// A function that FINISHES normally does not come through here at all: RET
// (opcode/direct_calls.cpp) pops the VM frame and jumps to the epilogue.
//

#include "../x64_generator.h"

using namespace asmjit;

namespace x64jit {

void X64Generator::emit_exits() {
    // NO save_stack_registers() in the two exits: everyone who jumps here has either just synced
    // (before the helper call that said "leave") or must not - after a call that
    // did not finish, the fiber's sp and ptr belong to the CALLEE on top of the
    // stack, and our copies in rbx / rbp are stale.
    a.bind(exitSwitch);
    a.mov(x86::eax, JIT_CONTEXT_SWITCH);
    a.jmp(leave);

    a.bind(exitKill);
    a.mov(x86::eax, JIT_KILL);
    a.jmp(leave);

    // the jump table sends "not an instruction" here, in register mode
    a.bind(declineFromRegisters);
    save_vm_registers();
    a.bind(decline);
    a.mov(x86::eax, JIT_CONTEXT_SWITCH);

    a.bind(leave);
    load_vm_registers();                                  // mov does not touch eax

    a.bind(epilogue);
    a.add(x86::rsp, FRAME_SIZE);                // the mirror image of the two body doors
    a.ret();

    // ---- register mode code that needs the resume check (a signal after a call)
    a.bind(resumeFromRegisters);
    save_vm_registers();
    a.jmp(resumeCheck);
}

} // namespace x64jit
