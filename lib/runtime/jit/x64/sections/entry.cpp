//
// Entry: how a compiled function is entered. There are three doors.
//
//      THE C++ DOOR  (jit_compiled_function::compiledCode)
//          int function(sharp_thread *thread, jit_compiled_function *fun, vm_cell *registers)
//      What the VM calls: prepare_method(), jit_enter_loop(), invoke_next_frame().
//      It is a small wrapper around the body:
//
//          save the six CPU registers we reserve (C++ expects them back)
//          thread, task, regs, fp, sp, ptr -> their CPU registers
//          if(task->rebuilding) run the frames above us first      (a fiber is being continued)
//          if(task->pc == task->rom and no signal is pending)
//                                      VM registers -> their homes; call the direct door
//          else                        call the resume door        (continue in the middle)
//          homes -> the VM register array                          (C++ reads the array)
//          restore, return what the body returned
//
//      THE DIRECT DOOR  (jit_compiled_function::directEntry)
//      What other compiled code calls (opcode/direct_calls.cpp). No arguments
//      and nothing to load: thread, task, fp, sp, ptr and the VM registers are
//      already in the CPU registers every compiled function agrees on. It is
//      the first instruction of the function, two machine instructions in.
//
//      THE RESUME DOOR
//      "Continue at task->pc": after a context switch, at an exception handler,
//      at the top of a hot loop taken over from the interpreter. Memory mode;
//      leads to the resume check (sections/dispatch.cpp).
//
// The body returns in REGISTER MODE with the return code in eax, to whichever
// door it was entered through (sections/exits.cpp).
//

#include "../x64_generator.h"

using namespace asmjit;

namespace x64jit {

void X64Generator::emit_entry() {
    Label notRebuilding = a.newLabel(), inTheMiddle = a.newLabel();
    Label bodyReturned = a.newLabel(), wrapperExit = a.newLabel();

    // ---------------------------------------------------------------- the C++ door
    a.push(x86::rbp);
    a.push(x86::rbx);
    a.push(x86::r12);
    a.push(x86::r13);
    a.push(x86::r14);
    a.push(x86::r15);
    // 6 pushes + the return address = 56 bytes; 8 more make rsp a multiple of 16
    a.sub(x86::rsp, 8);

    a.mov(THREAD, rdi);
    a.mov(REGS, rdx);
    a.mov(TASK, x86::qword_ptr(THREAD, offsetof(sharp_thread, task)));

    // if(task->rebuilding) { run the frames above this one; then see what happened }
    a.cmp(x86::byte_ptr(TASK, offsetof(fiber, rebuilding)), 0);
    a.je(notRebuilding);
    a.mov(rdi, (uint64_t)record);
    call_runtime(reinterpret_cast<void*>(jit_rt_rebuild_above));
    // The frames above us have run. If the function on top is still not us,
    // they did not finish (another switch): unwind again, nothing was loaded.
    a.mov(rax, fiber_field(offsetof(fiber, current)));
    a.mov(rcx, (uint64_t)function);
    a.cmp(rax, rcx);
    a.mov(x86::eax, JIT_CONTEXT_SWITCH);
    a.jne(wrapperExit);
    load_stack_registers();
    a.call(resumeEntry);
    a.jmp(bodyReturned);

    a.bind(notRebuilding);
    load_stack_registers();
    a.mov(rax, fiber_field(offsetof(fiber, pc)));
    a.cmp(rax, fiber_field(offsetof(fiber, rom)));
    a.jne(inTheMiddle);
    // pc at the first instruction is not always a fresh call: a function whose
    // first instruction is a CALL is continued THERE when the callee threw
    // (the pc stays on the call so the try block can be found). With a signal
    // pending take the resume door, which looks at it before anything runs;
    // the direct door would run the CALL a second time.
    a.cmp(thread_signal(), 0);
    a.jne(inTheMiddle);
    load_vm_registers();                              // -> register mode
    a.call(directEntry);
    a.jmp(bodyReturned);

    a.bind(inTheMiddle);                    // e.g. a hot loop taken over from the interpreter
    a.call(resumeEntry);

    a.bind(bodyReturned);
    save_vm_registers();                                // C++ reads the register array (eax is not a home)

    a.bind(wrapperExit);
    a.add(x86::rsp, 8);
    a.pop(x86::r15);
    a.pop(x86::r14);
    a.pop(x86::r13);
    a.pop(x86::r12);
    a.pop(x86::rbx);
    a.pop(x86::rbp);
    a.ret();

    // ---------------------------------------------------------------- the resume door
    a.bind(resumeEntry);
    a.sub(x86::rsp, FRAME_SIZE);
    a.jmp(resumeCheck);

    // ---------------------------------------------------------------- the direct door
    // (must stay last: the first instruction of the function follows)
    a.bind(directEntry);
    a.sub(x86::rsp, FRAME_SIZE);
}

} // namespace x64jit
