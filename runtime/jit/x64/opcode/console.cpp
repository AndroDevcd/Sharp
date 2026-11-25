//
// PUT / DPUT / PUTC / GET / SLEEP: straight calls into the runtime. None of
// them can raise an exception or look at the fiber, so there is no
// prepare_runtime_call(): only save_vm_registers() / load_vm_registers() around the call, because C++ does
// not preserve the homes of the VM registers.
//

#include "../x64_generator.h"

using namespace asmjit;

namespace x64jit {

emit_result X64Generator::emit_console() {
    switch(op) {
        case Opcode::PUT:       // cout << regs[a].i
            save_vm_registers();
            a.mov(rdi, vm_register_in_memory(single));
            call_runtime(reinterpret_cast<void*>(jit_rt_put_integer));
            load_vm_registers();
            return emitted;
        case Opcode::DPUT:      // cout << regs[a].d
            save_vm_registers();
            a.movsd(xmm0, vm_register_in_memory(single));
            call_runtime(reinterpret_cast<void*>(jit_rt_put_decimal));
            load_vm_registers();
            return emitted;
        case Opcode::PUTC:      // printf("%c", regs[a])
            save_vm_registers();
            a.mov(rdi, vm_register_in_memory(single));
            call_runtime(reinterpret_cast<void*>(jit_rt_put_character));
            load_vm_registers();
            return emitted;
        case Opcode::GET:       // regs[a] = regs[CMT] ? getche() : getch()
            save_vm_registers();
            a.mov(rdi, vm_register_in_memory(CMT));
            call_runtime(reinterpret_cast<void*>(jit_rt_get_character));
            load_vm_registers();
            store_vm_register(single, rax);
            return emitted;
        case Opcode::SLEEP:
            save_vm_registers();
            a.mov(rdi, vm_register_in_memory(single));
            call_runtime(reinterpret_cast<void*>(jit_rt_sleep));
            load_vm_registers();
            return emitted;

        default:
            return not_mine;
    }
}

} // namespace x64jit
