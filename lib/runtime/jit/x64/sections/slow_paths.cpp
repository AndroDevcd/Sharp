//
// Slow paths: code that is rarely run, placed after the instructions so the
// common path stays a straight line. An instruction asks for one with
// request_slow_path(kind, pc) and jumps to the label it gets.
//
//      slow_grow       a push found the stack full
//      slow_null       the object pointer (or the object) is null
//      slow_bounds     CHECKLEN failed (the index is in rcx)
//      slow_safepoint  a signal is pending at the bottom of a loop
//      slow_call       a direct call cannot be made (opcode/direct_calls.cpp):
//                      call through the runtime instead
//
// All of them are entered in REGISTER MODE and call into C++, so each starts
// with save_vm_registers(); the ones that come back end with load_vm_registers().
//

#include "../x64_generator.h"

using namespace asmjit;

namespace x64jit {

void X64Generator::emit_slow_paths() {
    for(slow_path_request &path : slowPaths) {
        a.bind(path.entry);
        save_vm_registers();                    // rax, rcx, rdx are not homes: an index in rcx survives
        set_current_instruction(path.pc);

        switch(path.kind) {
            case slow_grow:
                // the stack is full: grow it (or raise stack overflow), then push
                save_stack_registers();
                a.mov(rdi, THREAD);
                call_runtime_checked(reinterpret_cast<void*>(jit_rt_grow_stack));
                load_stack_registers();           // the stack may have moved
                load_vm_registers();
                a.jmp(path.back);
                break;

            case slow_null:
                save_stack_registers();
                a.mov(rdi, THREAD);
                call_runtime(reinterpret_cast<void*>(jit_rt_null_pointer));
                a.jmp(exception);
                break;

            case slow_bounds:       // the index is in rcx
                save_stack_registers();
                a.mov(rdi, THREAD);
                a.mov(rsi, rcx);
                call_runtime(reinterpret_cast<void*>(jit_rt_out_of_bounds));
                a.jmp(exception);
                break;

            case slow_safepoint:
                // the thread's signal word is set at the bottom of a loop;
                // path.pc is the top of the loop, where it continues
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
                load_vm_registers();
                a.jmp(labels[path.pc]);
                break;

            case slow_call:
                // prepare_method(address): compiles the callee when it is time,
                // grows the stack / the frame list, runs an interpreted callee
                save_stack_registers();
                a.mov(rdi, (int64_t)GET_Da(code[path.pc]));
                call_runtime(reinterpret_cast<void*>(jit_rt_call));
                check_after_runtime_call(true);
                a.jmp(path.back);   // the instruction after the CALL
                break;
        }
    }
}

} // namespace x64jit
