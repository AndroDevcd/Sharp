//
// The exception section: an exception is pending (the thread's tsig_except flag)
// and task->pc is the instruction it comes from. The interpreter's
// `catch_exception:`. A MEMORY MODE section.
//
//      switch(jit_rt_catch(thread)) {
//          1:  a try block of this function catches it; task->pc is the handler
//              -> continue there (resume check + dispatch)
//          2:  the thread was killed -> leave with JIT_KILL
//          0:  nothing here catches it -> pop our VM frame and return
//              JIT_EXCEPTION; the caller looks next
//      }
//

#include "../x64_generator.h"

using namespace asmjit;

namespace x64jit {

void X64Generator::emit_exception() {
    Label caught = a.newLabel();

    a.bind(exception);
    a.mov(rdi, THREAD);
    call_runtime(reinterpret_cast<void*>(jit_rt_catch));
    a.cmp(x86::eax, 1);
    a.je(caught);
    a.cmp(x86::eax, 2);
    a.je(exitKill);

    // nothing here catches it: the function is over
    a.mov(rdi, THREAD);
    a.mov(x86::esi, JIT_EXCEPTION);
    call_runtime(reinterpret_cast<void*>(jit_rt_leave));
    a.jmp(leave);                               // eax = JIT_EXCEPTION

    a.bind(caught);
    load_stack_registers();
    a.jmp(resumeCheck);
}

} // namespace x64jit
