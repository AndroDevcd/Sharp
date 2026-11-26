//
// Calls, returns, throwing and interrupts: the instructions that run other code.
//
// CALL and the normal RET have a fast form that stays in machine code
// (opcode/direct_calls.cpp). Everything else here goes through the runtime:
// prepare_runtime_call() makes the fiber current, and check_after_runtime_call() finds out
// what happened - the callee finished (carry on), it did not finish (leave with
// JIT_CONTEXT_SWITCH), or a signal is pending.
//

#include "../x64_generator.h"

using namespace asmjit;

namespace x64jit {

emit_result X64Generator::emit_calls() {
    switch(op) {
        case Opcode::CALL:      // prepare_method(a)
#ifdef SHARP_JIT_DIRECT_CALLS
            direct_call(single);
#else
            prepare_runtime_call();
            a.mov(rdi, (int64_t)single);
            call_runtime(reinterpret_cast<void*>(jit_rt_call));
            check_after_runtime_call(true);
#endif
            return emitted;
        case Opcode::CALLD:     // call_function_value(a): a = how many arguments were pushed
#ifdef SHARP_JIT_DIRECT_CALLS
            function_value_call(single);
            return emitted;
#endif
            prepare_runtime_call();
            a.mov(rdi, (int64_t)single);
            call_runtime(reinterpret_cast<void*>(jit_rt_call_dynamic));
            check_after_runtime_call(true);
            return emitted;
        case Opcode::INVOKE_DELEGATE: { // invoke_delegate(a, argument count, is static)
            const uint32_t second = code[pc + 1];
#ifdef SHARP_JIT_DIRECT_CALLS
            if(GET_Ca(second) == 0) {           // not static: the object's class decides
                virtual_call(GET_Cb(second));
                return emitted;
            }
#endif
            prepare_runtime_call();
            a.mov(rdi, (int64_t)single);
            a.mov(rsi, (int64_t)GET_Cb(second));
            a.mov(rdx, (int64_t)GET_Ca(second));
            call_runtime(reinterpret_cast<void*>(jit_rt_invoke_delegate));
            check_after_runtime_call(false);   // invoke_delegate moves task->pc itself: go where it points
            return emitted;
        }
        case Opcode::RET: {     // leave the function (a == 1: with the exception on the stack)
#ifdef SHARP_JIT_DIRECT_CALLS
            if(single != ERR_STATE) {
                direct_return();
                return emitted;
            }
#endif
            if(single == ERR_STATE) {
                // if(task->exceptionObject.o == nullptr) task->exceptionObject = pop;
                Label hasException = a.newLabel();
                a.cmp(fiber_field(offsetof(fiber, exceptionObject)), 0);
                a.jne(hasException);
                a.mov(rax, stack_object());
                pop_stack_slot();
                a.mov(fiber_field(offsetof(fiber, exceptionObject)), rax);
                a.bind(hasException);
            }
            save_stack_registers();
            save_vm_registers();
            a.mov(rdi, THREAD);
            a.mov(x86::esi, single == ERR_STATE ? JIT_EXCEPTION : JIT_OK);
            call_runtime(reinterpret_cast<void*>(jit_rt_leave));
            a.jmp(leave);       // eax = the code
            return emitted;
        }
        case Opcode::THROW:     // the exception object is on the stack
            prepare_runtime_call();
            a.mov(rdi, THREAD);
            call_runtime(reinterpret_cast<void*>(jit_rt_throw));
            a.jmp(exception);
            return emitted;
        case Opcode::INT:       // exec_interrupt(a): reads and writes the register array
            prepare_runtime_call();
            a.mov(rdi, (int64_t)single);
            call_runtime(reinterpret_cast<void*>(jit_rt_interrupt));
            a.test(x86::eax, x86::eax);         // the vm terminated
            a.jnz(exitKill);
            a.test(thread_signal(), 1 << tsig_except); // raised inside the interrupt: task->pc is still on it
            a.jnz(exception);
            set_current_instruction(next);                     // check_state(1)
            load_stack_registers();                           // an interrupt can push and pop
            a.cmp(thread_signal(), 0);
            a.jne(resumeCheck);
            load_vm_registers();
            return emitted;

        default:
            return not_mine;
    }
}

} // namespace x64jit
