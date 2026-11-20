//
// Rare code, kept out of the way of the instructions (jit/x64/sections/slow_paths.cpp): growing
// the stack, a null pointer, an index out of bounds, a loop's safe point, the write barrier.
//
#include "../arm_generator.h"

namespace armjit {

void ArmGenerator::emit_slow_paths() {
    // (indexed: a slow path asks for none, but the list must not change under a reference)
    for(size_t i = 0; i < slowPaths.size(); i++) {
        slow_path_request path = slowPaths[i];
        m.bind(path.entry);
        switch(path.kind) {
            case slow_grow:
                set_current_instruction(path.pc);
                save_stack_registers();
                m.call(reinterpret_cast<void*>(jit_rt_grow_stack), { Arg::pointer(THREAD) });
                jump_if_exception(exception);
                load_stack_registers();             // the stack may have moved
                m.jump(path.back);
                break;
            case slow_null:
                set_current_instruction(path.pc);
                save_stack_registers();
                m.call(reinterpret_cast<void*>(jit_rt_null_pointer), { Arg::pointer(THREAD) });
                m.jump(exception);
                break;
            case slow_bounds:       // the index is in V1
                set_current_instruction(path.pc);
                save_stack_registers();
                m.call(reinterpret_cast<void*>(jit_rt_out_of_bounds), { Arg::pointer(THREAD), Arg::value(V1) });
                m.jump(exception);
                break;
            case slow_safepoint:
                set_current_instruction(path.pc);
                save_stack_registers();
                m.call(reinterpret_cast<void*>(jit_rt_safepoint), { Arg::pointer(THREAD) });
                m.result_to_pointer(PA);
                m.p_jump_if_equals(PA, jit_exception, exception);
                m.p_jump_if_equals(PA, jit_switch, exitSwitch);
                m.p_jump_if_equals(PA, jit_kill, exitKill);
                load_stack_registers();
                m.jump(labels[path.pc]);
                break;
            case slow_barrier:
                m.p_load(PA, TASK, (int32_t) offsetof(fiber, ptrOwner));
                m.call(reinterpret_cast<void*>(jit_rt_remember), { Arg::pointer(PA), Arg::pointer(PTR) });
                m.jump(path.back);
                break;
        }
    }
}

} // namespace armjit
