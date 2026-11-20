//
// New objects (jit/x64/opcode/allocation.cpp): the runtime makes them, the result is pushed.
//
#include "../arm_generator.h"

namespace armjit {

emit_result ArmGenerator::emit_allocation() {
    switch(op) {
        case Opcode::NEWCLASS:
        case Opcode::NEWARRAY:
        case Opcode::NEWOBJARRAY:
        case Opcode::NEWCLASSARRAY:
        case Opcode::NEWSTRING: {
            prepare_runtime_call();
            switch(op) {
                case Opcode::NEWCLASS:      // create_object(&vm.classes[b])
                    call_checked(reinterpret_cast<void*>(jit_rt_new_class), { Arg::constant(raw) });
                    break;
                case Opcode::NEWARRAY:      // create_object(regs[a], (data_type)b)
                    load_register(V0, dual1);
                    call_checked(reinterpret_cast<void*>(jit_rt_new_number_array), { Arg::value(V0), Arg::constant(dual2) });
                    break;
                case Opcode::NEWOBJARRAY:   // create_object(regs[a])
                    load_register(V0, single);
                    call_checked(reinterpret_cast<void*>(jit_rt_new_object_array), { Arg::value(V0) });
                    break;
                case Opcode::NEWCLASSARRAY: // create_object(vm.classes + b, regs[a])
                    load_register(V0, single);
                    call_checked(reinterpret_cast<void*>(jit_rt_new_class_array), { Arg::constant(raw), Arg::value(V0) });
                    break;
                default:                    // the string constant a
                    call_checked(reinterpret_cast<void*>(jit_rt_new_string), { Arg::constant(single) });
                    break;
            }
            m.result_to_pointer(PA);
            m.keep_pointer(PA);                 // the push below may call the runtime
            push_stack_slot();
            m.kept_pointer(PA);
            m.p_store_object(PA, SP, slot_object(0));
            return emitted;
        }
        default:
            return not_mine;
    }
}

} // namespace armjit
