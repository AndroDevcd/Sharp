//
// The resume check and the dispatch: "continue at task->pc".
//
// Every instruction of a compiled function has an address in the function's
// jump table, so the function can be continued anywhere: after a context
// switch, at an exception handler, at the top of a hot loop the interpreter
// hands over, and for the computed jumps (BRH / IFE / IFNE).
//
//      resume check:   if(thread->signal) look at it first   (the interpreter's state_check:)
//                      rax = task->pc - task->rom             (in instructions)
//      dispatch:       VM registers -> their homes
//                      goto jumpTable[rax]
//
// Both are MEMORY MODE sections: whoever jumps here has the VM registers in the
// register array. Instructions run in register mode, so the homes are loaded on
// the way out.
//
// C++ picture: the interpreter's DISPATCH() - but only taken at these few
// moments, not after every instruction.
//

#include "../x64_generator.h"

using namespace asmjit;

namespace x64jit {

void X64Generator::emit_dispatch() {
    // ---- a direct call came back with something other than JIT_OK
    // (opcode/direct_calls.cpp). The callee left through `leave`, so the
    // register array is current. The same questions as after a slow call:
    a.bind(callNotFinished);
    a.mov(rax, fiber_field(offsetof(fiber, current)));
    a.mov(rcx, (uint64_t)function);
    a.cmp(rax, rcx);
    a.jne(exitSwitch);                          // the callee is still on the stack: unwind
    load_stack_registers();                                   // it is gone (an exception): look at the signal

    // ---- the resume check
    a.bind(resumeCheck);
    {
        Label quiet = a.newLabel();
        a.cmp(thread_signal(), 0);
        a.je(quiet);
        save_stack_registers();
        a.mov(rdi, THREAD);
        call_runtime(reinterpret_cast<void*>(jit_rt_safepoint));
        a.cmp(x86::eax, jit_exception);
        a.je(exception);
        a.cmp(x86::eax, jit_switch);
        a.je(exitSwitch);
        a.cmp(x86::eax, jit_kill);
        a.je(exitKill);
        load_stack_registers();
        a.bind(quiet);
        a.mov(rax, fiber_field(offsetof(fiber, pc)));
        a.sub(rax, fiber_field(offsetof(fiber, rom)));
        a.sar(rax, 2);                          // bytes -> instruction index
    }

    // ---- dispatch: goto the instruction whose pc is in rax
    a.bind(dispatch);
    a.cmp(rax, (int32_t)count);                 // unsigned: a negative pc is out of range too
    a.jae(decline);
    a.mov(rcx, (uint64_t)record);
    a.mov(rcx, x86::qword_ptr(rcx, offsetof(jit_compiled_function, jumpTable)));
    load_vm_registers();                                  // -> register mode
    a.jmp(x86::qword_ptr(rcx, rax, 3));
}

} // namespace x64jit
