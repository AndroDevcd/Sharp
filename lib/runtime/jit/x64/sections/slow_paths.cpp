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
//      slow_barrier    POPOBJ stored a young object into an old one that is not
//                      remembered yet (opcode/objects.cpp): remember it
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
                if(path.objectSlot >= 0) a.lea(PTR, local_object(path.objectSlot));      // (as `movl` would have)
                save_stack_registers();
                a.mov(rdi, THREAD);
                call_runtime(reinterpret_cast<void*>(jit_rt_null_pointer));
                a.jmp(exception);
                break;

            case slow_bounds:       // the index is in rcx
                if(path.objectSlot >= 0) a.lea(PTR, local_object(path.objectSlot));      // (the message names its size)
                save_stack_registers();
                a.mov(rdi, THREAD);
                a.mov(rsi, rcx);
                call_runtime(reinterpret_cast<void*>(jit_rt_out_of_bounds));
                a.jmp(exception);
                break;

            case slow_safepoint:
                // the thread's signal word is set at the bottom of a loop; path.pc is where it
                // continues: the top of the loop, or the jump itself (before_conditional_jump)
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

            case slow_virtual_call: {
                // the class is not in the place's cache, or its function cannot be called directly:
                // jit_rt_invoke_virtual looks it up, remembers it and calls it
                const uint32_t second = code[path.pc + 1];
                save_stack_registers();
                a.mov(rdi, (int64_t)GET_Da(code[path.pc]));
                a.mov(rsi, (int64_t)GET_Cb(second));
                a.mov(rdx, (uint64_t)path.data);
                call_runtime(reinterpret_cast<void*>(jit_rt_invoke_virtual));
                check_after_runtime_call(false);    // invoke_delegate moved task->pc: go where it points
                break;
            }

            case slow_found_call:
                // a function found while the code ran (a function value's) that cannot be called
                // directly: prepare_method(its address), as slow_call does for a known one
                save_stack_registers();
                a.mov(rax, machine_frame_slot(FRAME_CALLEE));
                a.movsxd(rdi, x86::dword_ptr(rax, offsetof(sharp_function, address)));
                call_runtime(reinterpret_cast<void*>(jit_rt_call));
                check_after_runtime_call(true);
                a.jmp(path.back);
                break;

            case slow_function_value_call:
                // a null function value or one with a bad address: the runtime raises
                save_stack_registers();
                a.mov(rdi, (int64_t)GET_Da(code[path.pc]));
                call_runtime(reinterpret_cast<void*>(jit_rt_call_dynamic));
                check_after_runtime_call(true);
                a.jmp(path.back);
                break;

            case slow_barrier:
                // gc_remember_store(task->ptrOwner, ptr). sp / fp / ptr are in registers
                // a C++ function leaves alone, and it does not read them from the fiber.
                a.mov(rdi, fiber_field(offsetof(fiber, ptrOwner)));
                a.mov(rsi, PTR);
                call_runtime(reinterpret_cast<void*>(jit_rt_remember));
                load_vm_registers();
                a.jmp(path.back);
                break;
        }
    }
}

} // namespace x64jit
