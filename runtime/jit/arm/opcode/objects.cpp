//
// The object pointer and references (jit/x64/opcode/objects.cpp).
//
#include "../arm_generator.h"

namespace armjit {

emit_result ArmGenerator::emit_objects() {
    switch(op) {
        case Opcode::MOVL:      // ptr = &(fp+a)->obj
            m.p_add(PTR, FP, slot_object(single));
            return emitted;
        case Opcode::MOVSL:     // ptr = &(sp+a)->obj
            m.p_add(PTR, SP, slot_object(single));
            return emitted;
        case Opcode::TLS_MOVL:  // ptr = &(task->stack+a)->obj
            m.p_load(PA, TASK, (int32_t) offsetof(fiber, stack));
            m.p_add(PTR, PA, slot_object(single));
            return emitted;
        case Opcode::MOVG:      // ptr = vm.staticHeap + a
            m.p_constant(PTR, (uintptr_t) (vm.staticHeap + single));
            return emitted;
        case Opcode::MOVN:      // ptr = ptr->o->node + b
            load_object_or_throw(PA);
            m.p_store(PA, TASK, (int32_t) offsetof(fiber, ptrOwner));
            m.p_load(PA, PA, (int32_t) offsetof(sharp_object, node));
            m.p_add(PTR, PA, (int32_t) (raw * 8));
            return emitted;
        case Opcode::MOVND:     // ptr = ptr->o->node + regs[a]
            load_object_or_throw(PA);
            m.p_store(PA, TASK, (int32_t) offsetof(fiber, ptrOwner));
            m.p_load(PA, PA, (int32_t) offsetof(sharp_object, node));
            load_register(V0, single);
            m.v_to_pointer(PB, V0);
            m.p_add_scaled(PTR, PA, PB, 3);
            return emitted;
        case Opcode::POPOBJ_2:  // ptr = &pop_stack_object
            m.p_add(PTR, SP, slot_object(0));
            pop_stack_slot();
            return emitted;
        case Opcode::PUSHOBJ:   // push *ptr
            push_stack_slot();
            load_object_or_null(PA);
            m.p_store_object(PA, SP, slot_object(0));
            return emitted;
        case Opcode::PUSHL:     // push (fp+a)->obj
            push_stack_slot();
            m.p_load(PA, FP, slot_object(single));
            m.p_store_object(PA, SP, slot_object(0));
            return emitted;
        case Opcode::PUSHNULL:
            push_stack_slot();
            m.p_constant(PA, 0);
            m.p_store_object(PA, SP, slot_object(0));
            return emitted;
        case Opcode::DUP:       // push the top object again
            push_stack_slot();
            m.p_load(PA, SP, slot_object(-1));
            m.p_store_object(PA, SP, slot_object(0));
            return emitted;
        case Opcode::POP:       // drop the top object
            m.p_constant(PA, 0);
            m.p_store_object(PA, SP, slot_object(0));
            pop_stack_slot();
            return emitted;
        case Opcode::POPL:      // (fp+a)->obj = pop
            m.p_load(PA, SP, slot_object(0));
            pop_stack_slot();
            m.p_store_object(PA, FP, slot_object(single));
            return emitted;
        case Opcode::POPOBJ: {  // *ptr = pop   (ptr must not be null)
            m.p_jump_if_zero(PTR, request_slow_path(slow_null, pc));
            m.p_load(PA, SP, slot_object(0));
            pop_stack_slot();
            m.p_store_object(PA, PTR, 0);
            // The write barrier (memory/tracing_collector.h): an old object that now refers to a
            // young one is remembered. The usual cases need no call:
            int done = m.label();
            int remember = request_slow_path(slow_barrier, pc, done);
            m.p_jump_if_zero(PA, done);
            m.p_load_32(PB, PA, (int32_t) offsetof(sharp_object, info));
            m.p_jump_if_bits(PB, 0x30000000, true, done);       // the value is not young
            m.p_load(PA, TASK, (int32_t) offsetof(fiber, ptrOwner));
            m.p_jump_if_zero(PA, done);
            m.p_load_32(PB, PA, (int32_t) offsetof(sharp_object, info));
            m.p_jump_if_bits(PB, 0x30000000, false, done);      // the owner is young
            m.p_load_8(PB, PA, (int32_t) offsetof(sharp_object, remembered));
            m.p_jump_if_zero(PB, remember);
            m.bind(done);
            return emitted;
        }
        case Opcode::SMOVR_3:   // (fp+a)->obj = *ptr
            load_object_or_null(PA);
            m.p_store_object(PA, FP, slot_object(single));
            return emitted;
        case Opcode::RETURNOBJ: // fp->obj = *ptr
            load_object_or_null(PA);
            m.p_store_object(PA, FP, slot_object(0));
            return emitted;
        case Opcode::DEL:       // *ptr = null
            m.p_constant(PA, 0);
            m.p_store_object(PA, PTR, 0);
            return emitted;
        case Opcode::CHECKNULL: { // regs[a] = ptr == null || *ptr == null
            int done = m.label();
            m.v_constant(V0, 1);
            m.p_jump_if_zero(PTR, done);
            m.p_load(PA, PTR, 0);
            m.p_jump_if_zero(PA, done);
            m.v_constant(V0, 0);
            m.bind(done);
            store_register(single, V0);
            return emitted;
        }
        case Opcode::SIZEOF: {  // regs[a] = the object's size, 0 for null
            int done = m.label();
            m.v_constant(V0, 0);
            m.p_jump_if_zero(PTR, done);
            m.p_load(PA, PTR, 0);
            m.p_jump_if_zero(PA, done);
            m.p_load_32(PB, PA, (int32_t) offsetof(sharp_object, size));
            m.v_from_pointer(V0, PB);
            m.bind(done);
            store_register(single, V0);
            return emitted;
        }
        case Opcode::CHECKLEN: { // 0 <= regs[a] < size, or a bounds exception
            load_object_or_throw(PA);
            int outside = request_slow_path(slow_bounds, pc);   // (it reads the index from V1)
            load_register(V1, single);
            m.p_load_32(PB, PA, (int32_t) offsetof(sharp_object, size));
            m.v_jump_if_not_below(V1, PB, outside);             // (a negative index is a huge one)
            return emitted;
        }
        case Opcode::ITEST:     // regs[a] = (the two popped objects are the same one)
            m.p_load(PA, SP, slot_object(0));
            m.p_load(PB, SP, slot_object(-1));
            m.p_add(SP, SP, -32);
            m.v_from_pointer(V0, PA);
            m.v_from_pointer(V1, PB);
            m.v_compare(V0, V0, V1, EQ);
            store_register(single, V0);
            return emitted;
        default:
            return not_mine;
    }
}

} // namespace armjit
