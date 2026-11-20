//
// Calls, returns, throw and interrupts (jit/x64/opcode/calls.cpp). CALL, CALLD, a call of an
// overridden function and the normal RET stay in machine code (opcode/direct_calls.cpp); the
// rest goes through the runtime.
//
#include "../arm_generator.h"

namespace armjit {

emit_result ArmGenerator::emit_calls() {
    switch(op) {
        case Opcode::CALL:      // prepare_method(a)
#ifdef SHARP_ARM_JIT_DIRECT_CALLS
            direct_call(single);
#else
            prepare_runtime_call();
            m.call(reinterpret_cast<void*>(jit_rt_call), { Arg::constant(single) });
            check_after_runtime_call(true);
#endif
            return emitted;
        case Opcode::CALLD:     // call_function_value(a): a = how many arguments were pushed
#ifdef SHARP_ARM_JIT_DIRECT_CALLS
            function_value_call(single);
#else
            prepare_runtime_call();
            m.call(reinterpret_cast<void*>(jit_rt_call_dynamic), { Arg::constant(single) });
            check_after_runtime_call(true);
#endif
            return emitted;
        case Opcode::INVOKE_DELEGATE: { // invoke_delegate(a, argument count, is static)
            const uint32_t second = code[pc + 1];
#ifdef SHARP_ARM_JIT_DIRECT_CALLS
            if(GET_Ca(second) == 0) {           // not static: the object's class decides
                virtual_call(GET_Cb(second));
                return emitted;
            }
#endif
            prepare_runtime_call();
            m.call(reinterpret_cast<void*>(jit_rt_invoke_delegate),
                   { Arg::constant(single), Arg::constant(GET_Cb(second)), Arg::constant(GET_Ca(second)) });
            check_after_runtime_call(false);    // invoke_delegate moves task->pc itself: go where it points
            return emitted;
        }
        case Opcode::RET: {     // leave the function (a == 1: with the exception on the stack)
#ifdef SHARP_ARM_JIT_DIRECT_CALLS
            if(single != ERR_STATE) {
                direct_return();
                return emitted;
            }
#endif
            if(single == ERR_STATE) {
                // if(task->exceptionObject.o == nullptr) task->exceptionObject = pop
                int hasException = m.label();
                m.p_load(PA, TASK, (int32_t) offsetof(fiber, exceptionObject));
                m.p_jump_if_not_zero(PA, hasException);
                m.p_load(PA, SP, slot_object(0));
                pop_stack_slot();
                m.p_store_object(PA, TASK, (int32_t) offsetof(fiber, exceptionObject));
                m.bind(hasException);
            }
            save_stack_registers();
            m.call(reinterpret_cast<void*>(jit_rt_leave),
                   { Arg::pointer(THREAD), Arg::constant(single == ERR_STATE ? JIT_EXCEPTION : JIT_OK) });
            m.jump(leave);      // what it returned is the return code
            return emitted;
        }
        case Opcode::THROW:     // the exception object is on the stack
            prepare_runtime_call();
            m.call(reinterpret_cast<void*>(jit_rt_throw), { Arg::pointer(THREAD) });
            m.jump(exception);
            return emitted;
        case Opcode::INT:       // exec_interrupt(a): reads and writes the register array
            prepare_runtime_call();
            m.call(reinterpret_cast<void*>(jit_rt_interrupt), { Arg::constant(single) });
            m.result_to_pointer(PA);
            m.p_jump_if_not_zero(PA, exitKill);         // the vm terminated
            jump_if_exception(exception);               // raised inside the interrupt: task->pc is still on it
            set_current_instruction(next);              // check_state(1)
            load_stack_registers();                     // an interrupt can push and pop
            jump_if_signal(resumeCheck);
            return emitted;
        default:
            return not_mine;
    }
}

} // namespace armjit
