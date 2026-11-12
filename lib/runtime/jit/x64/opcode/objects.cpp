//
// The object pointer (ptr, kept in rbp) and references.
//
// ptr is the address of a SLOT that holds a reference (a local, a field, a
// static), not the object itself: MOVL / MOVG / MOVN point it somewhere, the
// other instructions read or write the reference in that slot.
//
// Storing a reference is a plain 8-byte store: the tracing collector needs no
// bookkeeping on assignment.
//

#include "../x64_generator.h"

using namespace asmjit;

namespace x64jit {

emit_result X64Generator::emit_objects() {
    switch(op) {
        case Opcode::MOVL:      // ptr = &(fp+a)->obj
            a.lea(PTR, local_object(single));
            return emitted;
        case Opcode::MOVSL:     // ptr = &(sp+a)->obj
            a.lea(PTR, stack_object(single));
            return emitted;
        case Opcode::TLS_MOVL:  // ptr = &(task->stack+a)->obj
            a.mov(rax, fiber_field(offsetof(fiber, stack)));
            a.lea(PTR, x86::qword_ptr(rax, (int32_t)(single * 16 + 8)));
            return emitted;
        case Opcode::MOVG:      // ptr = vm.staticHeap + a
            a.mov(PTR, (uint64_t)(vm.staticHeap + single));
            return emitted;
        case Opcode::MOVN:      // ptr = ptr->o->node + b
            load_object_or_throw();
            a.mov(rax, x86::qword_ptr(rax, offsetof(sharp_object, node)));
            a.lea(PTR, x86::qword_ptr(rax, (int32_t)(raw * 8)));
            return emitted;
        case Opcode::MOVND:     // ptr = ptr->o->node + regs[a]
            load_object_or_throw();
            a.mov(rax, x86::qword_ptr(rax, offsetof(sharp_object, node)));
            load_vm_register(rcx, single);
            a.lea(PTR, x86::qword_ptr(rax, rcx, 3));
            return emitted;
        case Opcode::POPOBJ_2:  // ptr = &pop_stack_object
            a.lea(PTR, stack_object());
            pop_stack_slot();
            return emitted;

        // ============================ references ===============================
        case Opcode::PUSHOBJ:   // push *ptr
            push_stack_slot();
            load_object_or_null();
            a.mov(stack_object(), rax);
            return emitted;
        case Opcode::PUSHL:     // push (fp+a)->obj
            push_stack_slot();
            a.mov(rax, local_object(single));
            a.mov(stack_object(), rax);
            return emitted;
        case Opcode::PUSHNULL:
            push_stack_slot();
            a.mov(stack_object(), 0);
            return emitted;
        case Opcode::DUP:       // push the top object again
            push_stack_slot();
            a.mov(rax, stack_object(-1));
            a.mov(stack_object(), rax);
            return emitted;
        case Opcode::POP:       // drop the top object
            a.mov(stack_object(), 0);
            pop_stack_slot();
            return emitted;
        case Opcode::POPL:      // (fp+a)->obj = pop
            a.mov(rax, stack_object());
            pop_stack_slot();
            a.mov(local_object(single), rax);
            return emitted;
        case Opcode::POPOBJ: {  // *ptr = pop   (ptr must not be null)
            a.test(PTR, PTR);
            a.jz(request_slow_path(slow_null, pc));
            a.mov(rax, stack_object());
            pop_stack_slot();
            a.mov(x86::qword_ptr(PTR), rax);
            return emitted;
        }
        case Opcode::SMOVR_3:   // (fp+a)->obj = *ptr
            load_object_or_null();
            a.mov(local_object(single), rax);
            return emitted;
        case Opcode::RETURNOBJ: // fp->obj = *ptr
            load_object_or_null();
            a.mov(local_object(0), rax);
            return emitted;
        case Opcode::DEL:       // *ptr = null
            a.mov(x86::qword_ptr(PTR), 0);
            return emitted;
        case Opcode::CHECKNULL: { // regs[a] = ptr == null || *ptr == null
            Label done = a.newLabel();
            a.mov(x86::eax, 1);
            a.test(PTR, PTR);
            a.jz(done);
            a.cmp(x86::qword_ptr(PTR), 0);
            a.je(done);
            a.xor_(x86::eax, x86::eax);
            a.bind(done);
            store_vm_register(single, rax);
            return emitted;
        }
        case Opcode::SIZEOF: {  // regs[a] = the object's size, 0 for null
            Label done = a.newLabel();
            a.xor_(x86::eax, x86::eax);
            a.test(PTR, PTR);
            a.jz(done);
            a.mov(rcx, x86::qword_ptr(PTR));
            a.test(rcx, rcx);
            a.jz(done);
            a.mov(x86::eax, x86::dword_ptr(rcx, offsetof(sharp_object, size)));
            a.bind(done);
            store_vm_register(single, rax);
            return emitted;
        }
        case Opcode::CHECKLEN:  // 0 <= regs[a] < size, or a bounds exception
            load_object_or_throw();
            load_vm_register(rcx, single);
            a.mov(x86::edx, x86::dword_ptr(rax, offsetof(sharp_object, size)));
            a.cmp(rcx, rdx);                    // unsigned: a negative index is a huge one
            a.jae(request_slow_path(slow_bounds, pc));
            return emitted;
        case Opcode::ITEST:     // regs[a] = (the two popped objects are the same one)
            a.mov(rax, stack_object());
            a.cmp(rax, stack_object(-1));
            a.lea(SP, x86::qword_ptr(SP, -32)); // pop both; lea leaves the flags of the cmp alone
            store_condition(single, &X64Generator::condition_equal);
            return emitted;
        default:
            return not_mine;
    }
}

} // namespace x64jit
