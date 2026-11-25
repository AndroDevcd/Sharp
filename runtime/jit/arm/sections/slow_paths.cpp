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
            case slow_safepoint:        // path.pc: where it continues - the top of the loop, or the jump itself
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
            case slow_call:
                // prepare_method(address): compiles the callee when it is time, grows the stack /
                // the frame list, runs an interpreted callee
                set_current_instruction(path.pc);
                save_stack_registers();
                m.call(reinterpret_cast<void*>(jit_rt_call), { Arg::constant(GET_Da(code[path.pc])) });
                check_after_runtime_call(true);
                m.jump(path.back);                  // the instruction after the CALL
                break;
            case slow_virtual_call: {
                // the class is not in the place's cache, or its function cannot be called directly:
                // jit_rt_invoke_virtual looks it up, remembers it and calls it
                const uint32_t second = code[path.pc + 1];
                set_current_instruction(path.pc);
                save_stack_registers();
                m.p_constant(PA, (uintptr_t) path.data);
                m.call(reinterpret_cast<void*>(jit_rt_invoke_virtual),
                       { Arg::constant(GET_Da(code[path.pc])), Arg::constant(GET_Cb(second)), Arg::pointer(PA) });
                check_after_runtime_call(false);    // invoke_delegate moved task->pc: go where it points
                break;
            }
            case slow_found_call:
                // a function found while the code ran (a function value's) that cannot be called
                // directly: prepare_method(its address), as slow_call does for a known one
                set_current_instruction(path.pc);
                save_stack_registers();
                m.frame_load(PA, FRAME_KEPT);
                m.p_load_int(PA, PA, (int32_t) offsetof(sharp_function, address));
                m.v_from_pointer(V0, PA);
                m.call(reinterpret_cast<void*>(jit_rt_call), { Arg::value(V0) });
                check_after_runtime_call(true);
                m.jump(path.back);
                break;
            case slow_function_value_call:
                // a null function value or one with a bad address: the runtime raises
                set_current_instruction(path.pc);
                save_stack_registers();
                m.call(reinterpret_cast<void*>(jit_rt_call_dynamic), { Arg::constant(GET_Da(code[path.pc])) });
                check_after_runtime_call(true);
                m.jump(path.back);
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
