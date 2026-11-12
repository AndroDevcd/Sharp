//
// Casts, type tests, SWAP and the object lock. All of them are calls into the
// runtime (jit_runtime.h): prepare_runtime_call(), the call, load_vm_registers().
//

#include "../x64_generator.h"

using namespace asmjit;

namespace x64jit {

emit_result X64Generator::emit_casts_and_locks() {
    switch(op) {
        case Opcode::CAST:      // cast_object(ptr, a)
            prepare_runtime_call();
            a.mov(rdi, (int64_t)single);
            call_runtime_checked(reinterpret_cast<void*>(jit_rt_cast));
            load_vm_registers();
            return emitted;
        case Opcode::VARCAST:   // cast_numeric_array(ptr, a)
            prepare_runtime_call();
            a.mov(rdi, (int64_t)dual1);
            call_runtime_checked(reinterpret_cast<void*>(jit_rt_cast_number_array));
            load_vm_registers();
            return emitted;
        case Opcode::IS:        // regs[a] = is_type(ptr, b)
            prepare_runtime_call();
            a.mov(rdi, (int64_t)raw);
            call_runtime_checked(reinterpret_cast<void*>(jit_rt_is));
            load_vm_registers();
            store_vm_register(single, rax);
            return emitted;
        case Opcode::SWAP:      // swap the two slots on top of the stack
            prepare_runtime_call();
            a.mov(rdi, THREAD);
            call_runtime_checked(reinterpret_cast<void*>(jit_rt_swap));
            load_vm_registers();
            return emitted;
        case Opcode::LOCK:      // if(lock_object(ptr->o)) return;   (the fiber waits, and runs LOCK again)
            prepare_runtime_call();
            a.mov(rdi, THREAD);
            call_runtime_checked(reinterpret_cast<void*>(jit_rt_lock));
            a.test(x86::eax, x86::eax);
            a.jnz(exitSwitch);                  // task->pc is this LOCK
            load_vm_registers();
            return emitted;
        case Opcode::ULOCK:
            prepare_runtime_call();
            a.mov(rdi, THREAD);
            call_runtime_checked(reinterpret_cast<void*>(jit_rt_unlock));
            load_vm_registers();
            return emitted;

        default:
            return not_mine;
    }
}

} // namespace x64jit
