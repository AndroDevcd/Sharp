//
// Casts, `is`, swap, lock and unlock (jit/x64/opcode/casts_and_locks.cpp): all done by the runtime.
//
#include "../arm_generator.h"

namespace armjit {

emit_result ArmGenerator::emit_casts_and_locks() {
    switch(op) {
        case Opcode::CAST:      // cast_object(ptr, a)
            prepare_runtime_call();
            call_checked(reinterpret_cast<void*>(jit_rt_cast), { Arg::constant(single) });
            return emitted;
        case Opcode::VARCAST:   // cast_numeric_array(ptr, a)
            prepare_runtime_call();
            call_checked(reinterpret_cast<void*>(jit_rt_cast_number_array), { Arg::constant(dual1) });
            return emitted;
        case Opcode::IS:        // regs[a] = is_type(ptr, b)
            prepare_runtime_call();
            call_checked(reinterpret_cast<void*>(jit_rt_is), { Arg::constant(raw) });
            m.result_to_value(V0);
            store_register(single, V0);
            return emitted;
        case Opcode::SWAP:      // swap the two slots on top of the stack
            prepare_runtime_call();
            call_checked(reinterpret_cast<void*>(jit_rt_swap), { Arg::pointer(THREAD) });
            m.stack_numbers_changed();
            return emitted;
        case Opcode::LOCK:      // if(lock_object(ptr->o)) return;   (the fiber waits, and runs LOCK again)
            prepare_runtime_call();
            call_checked(reinterpret_cast<void*>(jit_rt_lock), { Arg::pointer(THREAD) });
            m.result_to_pointer(PA);
            m.p_jump_if_not_zero(PA, exitSwitch);       // task->pc is this LOCK
            return emitted;
        case Opcode::ULOCK:
            prepare_runtime_call();
            call_checked(reinterpret_cast<void*>(jit_rt_unlock), { Arg::pointer(THREAD) });
            return emitted;
        default:
            return not_mine;
    }
}

} // namespace armjit
