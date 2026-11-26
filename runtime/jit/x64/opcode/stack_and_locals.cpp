//
// Numbers moving between VM registers, the locals of the frame (fp + n) and the
// top of the stack (sp), and arithmetic done straight on a local.
//
// A stack slot is 16 bytes: the number at +0, the object reference at +8.
// Locals and the stack are memory (the fiber's stack); the VM registers are
// mostly CPU registers (their homes), so a load or store is one instruction:
//
//      loadl ebx, fp+2            mov rsi, [r15 + 2*16]
//      smovr_2 ebx, fp+2          mov [r15 + 2*16], rsi
//      addl ecx, fp+3             add [r15 + 3*16], r9
//

#include "../x64_generator.h"

using namespace asmjit;

namespace x64jit {

emit_result X64Generator::emit_stack_and_locals() {
    switch(op) {
        // ---- the stack
        case Opcode::RSTORE:    // push regs[a]
            push_stack_slot();
            a.mov(stack_number(), vm_register_value(single, rax));
            return emitted;
        case Opcode::LOADVAL:   // regs[a] = pop
            if(has_cpu_register(single)) a.mov(cpu_register_of(single), stack_number());
            else { a.mov(rax, stack_number()); store_vm_register(single, rax); }
            pop_stack_slot();
            return emitted;
        case Opcode::ISTORE:    // push b
            push_stack_slot();
            a.mov(stack_number(), raw);
            return emitted;
        case Opcode::DISTORE:   // push (double)b
            push_stack_slot();
            a.mov(rax, bits_of((double)raw));
            a.mov(stack_number(), rax);
            return emitted;
        case Opcode::SMOVR:     // (sp+b)->var = regs[a]
            a.mov(stack_number(dual2), vm_register_value(dual1, rax));
            return emitted;
        case Opcode::SMOV:      // regs[a] = (sp+b)->var
            if(has_cpu_register(dual1)) a.mov(cpu_register_of(dual1), stack_number(dual2));
            else { a.mov(rax, stack_number(dual2)); store_vm_register(dual1, rax); }
            return emitted;
        case Opcode::ISADD:     // (sp+a)->var += b
            a.add(stack_number(single), raw);
            return emitted;
        case Opcode::DISADD:    // (sp+a)->var += (double)b
            load_decimal_constant((double)raw);
            a.movsd(xmm0, stack_number(single)); a.addsd(xmm0, xmm1); a.movsd(stack_number(single), xmm0);
            return emitted;

        // ---- locals
        case Opcode::LOADL:     // regs[a] = (fp+b)->var
            store_vm_register(dual1, slot_in_register(dual2, rax));
            return emitted;
        case Opcode::SMOVR_2:   // (fp+b)->var = regs[a]
            store_slot(dual2, vm_register_value(dual1, rax));
            return emitted;
        case Opcode::SMOVR_4:   // (fp+a)->var = (fp+b)->var
            // (a decimal that is at home in a register goes from register to register: through a general
            //  purpose register it would cross over twice, and a copy in a loop is on the way of everything after it)
            if(decimal_home(single) >= 0 || decimal_home(raw) >= 0) store_slot_decimal(single, slot_in_decimal_register(raw, xmm0));
            else store_slot(single, slot_in_register(raw, rax));
            return emitted;
        case Opcode::ISTOREL:   // (fp+a)->var = b
            store_slot_constant(single, (uint64_t)(int64_t)raw);
            return emitted;
        case Opcode::DISTOREL:  // (fp+a)->var = (double)b
            store_slot_constant(single, bits_of((double)raw));
            return emitted;
        case Opcode::IPUSHL:    // push (fp+a)->var
            push_stack_slot();
            a.mov(rax, local_number(single));
            a.mov(stack_number(), rax);
            return emitted;
        case Opcode::IPOPL:     // (fp+a)->var = pop
            a.mov(rax, stack_number());
            pop_stack_slot();
            a.mov(local_number(single), rax); sync_slot_home(single);
            return emitted;
        case Opcode::RETURNVAL: // fp->var = regs[a]
            a.mov(local_number(0), vm_register_value(single, rax)); sync_slot_home(0);
            return emitted;

        // ---- thread locals: slots at the bottom of the fiber's stack
        case Opcode::MOV_ABS:   // (task->stack+a)->var = regs[EBX]
            a.mov(rcx, fiber_field(offsetof(fiber, stack)));
            a.mov(x86::qword_ptr(rcx, (int32_t)(single * 16)), vm_register_value(EBX, rax));
            return emitted;
        case Opcode::LOAD_ABS:  // regs[EBX] = (task->stack+a)->var
            a.mov(rcx, fiber_field(offsetof(fiber, stack)));
            a.mov(rax, x86::qword_ptr(rcx, (int32_t)(single * 16)));
            store_vm_register(EBX, rax);
            return emitted;

        // ---- integer arithmetic straight on a local
        case Opcode::ADDL:      // (fp+b)->var += regs[a]
            a.add(local_number(dual2), vm_register_value(dual1, rax)); sync_slot_home(dual2);
            return emitted;
        case Opcode::SUBL:
            a.sub(local_number(dual2), vm_register_value(dual1, rax)); sync_slot_home(dual2);
            return emitted;
        case Opcode::MULL:
            a.mov(rax, local_number(dual2));
            a.emit(x86::Inst::kIdImul, rax, vm_register(dual1));
            a.mov(local_number(dual2), rax); sync_slot_home(dual2);
            return emitted;
        case Opcode::MODL: {    // (fp+b)->var = vm_remainder(local, regs[a])
            Label zero = a.newLabel(), done = a.newLabel();
            a.emit(x86::Inst::kIdMov, rcx, vm_register(dual1));
            a.mov(rax, local_number(dual2));
            divide_for_remainder(zero);
            a.mov(local_number(dual2), rdx); sync_slot_home(dual2);
            a.jmp(done);
            a.bind(zero);                       // a divisor of 0: the runtime raises
            prepare_runtime_call();
            a.mov(rdi, local_number(dual2));
            a.mov(rsi, vm_register_in_memory(dual1));
            call_runtime_checked(reinterpret_cast<void*>(jit_rt_remainder));
            load_vm_registers();
            a.mov(local_number(dual2), rax); sync_slot_home(dual2);
            a.bind(done);
            return emitted;
        }
        case Opcode::ANDL:
            a.and_(local_number(dual2), vm_register_value(dual1, rax)); sync_slot_home(dual2);
            return emitted;
        case Opcode::ORL:
            a.or_(local_number(dual2), vm_register_value(dual1, rax)); sync_slot_home(dual2);
            return emitted;
        case Opcode::XORL:
            a.xor_(local_number(dual2), vm_register_value(dual1, rax)); sync_slot_home(dual2);
            return emitted;
        case Opcode::IADDL:     // (fp+a)->var += b
            if(whole_home(single) >= 0) {       // `i++` with i at home
                a.add(whole_home_register(whole_home(single)), raw);
                a.mov(local_number(single), whole_home_register(whole_home(single)));
            } else { a.add(local_number(single), raw); sync_slot_home(single); }
            return emitted;
        case Opcode::ISUBL:
            if(whole_home(single) >= 0) {
                a.sub(whole_home_register(whole_home(single)), raw);
                a.mov(local_number(single), whole_home_register(whole_home(single)));
            } else { a.sub(local_number(single), raw); sync_slot_home(single); }
            return emitted;
        case Opcode::IMULL:
            a.mov(rax, local_number(single));
            a.imul(rax, rax, raw);
            a.mov(local_number(single), rax); sync_slot_home(single);
            return emitted;
        case Opcode::IMODL:
            if(raw != 0) {                      // (0: the runtime raises, below)
                a.mov(rax, local_number(single));
                constant_remainder((int64_t)raw);
                a.mov(local_number(single), rdx); sync_slot_home(single);
                return emitted;
            }
            prepare_runtime_call();
            a.mov(rdi, local_number(single));
            a.mov(rsi, (int64_t)raw);
            call_runtime_checked(reinterpret_cast<void*>(jit_rt_remainder));
            load_vm_registers();
            a.mov(local_number(single), rax); sync_slot_home(single);
            return emitted;

        // ---- decimal arithmetic straight on a local
        case Opcode::DADDL:     // (fp+b)->var.d += regs[a].d
            a.movsd(xmm0, local_number(dual2)); a.addsd(xmm0, vm_register_decimal(dual1, xmm1)); a.movsd(local_number(dual2), xmm0); sync_slot_home(dual2);
            return emitted;
        case Opcode::DSUBL:
            a.movsd(xmm0, local_number(dual2)); a.subsd(xmm0, vm_register_decimal(dual1, xmm1)); a.movsd(local_number(dual2), xmm0); sync_slot_home(dual2);
            return emitted;
        case Opcode::DMULL:
            a.movsd(xmm0, local_number(dual2)); a.mulsd(xmm0, vm_register_decimal(dual1, xmm1)); a.movsd(local_number(dual2), xmm0); sync_slot_home(dual2);
            return emitted;
        case Opcode::DDIVL:
            a.movsd(xmm0, local_number(dual2)); a.divsd(xmm0, vm_register_decimal(dual1, xmm1)); a.movsd(local_number(dual2), xmm0); sync_slot_home(dual2);
            return emitted;
        case Opcode::DIADDL:    // (fp+a)->var.d += (double)b
            load_decimal_constant((double)raw);
            a.movsd(xmm0, local_number(single)); a.addsd(xmm0, xmm1); a.movsd(local_number(single), xmm0); sync_slot_home(single);
            return emitted;
        case Opcode::DISUBL:
            load_decimal_constant((double)raw);
            a.movsd(xmm0, local_number(single)); a.subsd(xmm0, xmm1); a.movsd(local_number(single), xmm0); sync_slot_home(single);
            return emitted;
        case Opcode::DIMULL:
            load_decimal_constant((double)raw);
            a.movsd(xmm0, local_number(single)); a.mulsd(xmm0, xmm1); a.movsd(local_number(single), xmm0); sync_slot_home(single);
            return emitted;
        case Opcode::DIDIVL:
            load_decimal_constant((double)raw);
            a.movsd(xmm0, local_number(single)); a.divsd(xmm0, xmm1); a.movsd(local_number(single), xmm0); sync_slot_home(single);
            return emitted;

        default:
            return not_mine;
    }
}

} // namespace x64jit
