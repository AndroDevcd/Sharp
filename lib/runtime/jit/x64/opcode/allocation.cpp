//
// New objects. The runtime makes the object (jit_rt_new_...), compiled code
// pushes the reference.
//
// The arguments are read from the register ARRAY: prepare_runtime_call() has written
// the homes there, and rdi / rsi - the argument registers - are homes
// themselves (see "Calling C++" in x64_generator.cpp).
//

#include "../x64_generator.h"

using namespace asmjit;

namespace x64jit {

emit_result X64Generator::emit_allocation() {
    switch(op) {
        case Opcode::NEWCLASS:
        case Opcode::NEWARRAY:
        case Opcode::NEWOBJARRAY:
        case Opcode::NEWCLASSARRAY:
        case Opcode::NEWSTRING: {
            prepare_runtime_call();
            void *helper;
            switch(op) {
                case Opcode::NEWCLASS:      // create_object(&vm.classes[b])
                    a.mov(rdi, (int64_t)raw);
                    helper = reinterpret_cast<void*>(jit_rt_new_class);
                    break;
                case Opcode::NEWARRAY:      // create_object(regs[a], (data_type)b)
                    a.mov(rdi, vm_register_in_memory(dual1));
                    a.mov(rsi, (int64_t)dual2);
                    helper = reinterpret_cast<void*>(jit_rt_new_number_array);
                    break;
                case Opcode::NEWOBJARRAY:   // create_object(regs[a])
                    a.mov(rdi, vm_register_in_memory(single));
                    helper = reinterpret_cast<void*>(jit_rt_new_object_array);
                    break;
                case Opcode::NEWCLASSARRAY: // create_object(vm.classes + b, regs[a])
                    a.mov(rdi, (int64_t)raw);
                    a.mov(rsi, vm_register_in_memory(single));
                    helper = reinterpret_cast<void*>(jit_rt_new_class_array);
                    break;
                default:                    // the string constant a
                    a.mov(rdi, (int64_t)single);
                    helper = reinterpret_cast<void*>(jit_rt_new_string);
                    break;
            }
            call_runtime_checked(helper);
            load_vm_registers();
            a.mov(machine_frame_slot(FRAME_SCRATCH), rax);   // the push below may call a helper
            push_stack_slot();
            a.mov(rax, machine_frame_slot(FRAME_SCRATCH));
            a.mov(stack_object(), rax);
            return emitted;
        }

        default:
            return not_mine;
    }
}

} // namespace x64jit
