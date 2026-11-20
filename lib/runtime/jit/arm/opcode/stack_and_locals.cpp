//
// Numbers moving between registers, locals and the stack (jit/x64/opcode/stack_and_locals.cpp).
//
#include "../arm_generator.h"

namespace armjit {

emit_result ArmGenerator::emit_stack_and_locals() {
    switch(op) {
        case Opcode::RSTORE:    // push regs[a]
            push_stack_slot();
            load_register(V0, single);
            m.v_store(V0, SP, slot_number(0));
            return emitted;
        case Opcode::LOADVAL:   // regs[a] = pop
            m.v_load(V0, SP, slot_number(0));
            store_register(single, V0);
            pop_stack_slot();
            return emitted;
        case Opcode::ISTORE:    // push b
            push_stack_slot();
            m.v_constant(V0, (int64_t) raw);
            m.v_store(V0, SP, slot_number(0));
            return emitted;
        case Opcode::DISTORE:   // push (double)b
            push_stack_slot();
            m.v_constant(V0, bits_of((double) raw));
            m.v_store(V0, SP, slot_number(0));
            return emitted;
        case Opcode::SMOVR:     // (sp+b)->var = regs[a]
            load_register(V0, dual1);
            m.v_store(V0, SP, slot_number(dual2));
            return emitted;
        case Opcode::SMOV:      // regs[a] = (sp+b)->var
            m.v_load(V0, SP, slot_number(dual2));
            store_register(dual1, V0);
            return emitted;
        case Opcode::ISADD:     // (sp+a)->var += b
            m.v_load(V0, SP, slot_number(single));
            m.v_constant(V1, (int64_t) raw);
            m.v_operation(V_ADD, V0, V0, V1);
            m.v_store(V0, SP, slot_number(single));
            return emitted;
        case Opcode::DISADD:    // (sp+a)->var += (double)b
            m.d_load(D0, SP, slot_number(single));
            m.d_constant(D1, (double) raw);
            m.d_operation(D_ADD, D0, D0, D1);
            m.d_store(D0, SP, slot_number(single));
            return emitted;
        case Opcode::LOADL:     // regs[a] = (fp+b)->var
            m.v_load(V0, FP, slot_number(dual2));
            store_register(dual1, V0);
            return emitted;
        case Opcode::SMOVR_2:   // (fp+b)->var = regs[a]
            load_register(V0, dual1);
            m.v_store(V0, FP, slot_number(dual2));
            return emitted;
        case Opcode::SMOVR_4:   // (fp+a)->var = (fp+b)->var
            m.v_load(V0, FP, slot_number(raw));
            m.v_store(V0, FP, slot_number(single));
            return emitted;
        case Opcode::ISTOREL:   // (fp+a)->var = b
            m.v_constant(V0, (int64_t) raw);
            m.v_store(V0, FP, slot_number(single));
            return emitted;
        case Opcode::DISTOREL:  // (fp+a)->var = (double)b
            m.v_constant(V0, bits_of((double) raw));
            m.v_store(V0, FP, slot_number(single));
            return emitted;
        case Opcode::IPUSHL:    // push (fp+a)->var
            push_stack_slot();
            m.v_load(V0, FP, slot_number(single));
            m.v_store(V0, SP, slot_number(0));
            return emitted;
        case Opcode::IPOPL:     // (fp+a)->var = pop
            m.v_load(V0, SP, slot_number(0));
            pop_stack_slot();
            m.v_store(V0, FP, slot_number(single));
            return emitted;
        case Opcode::RETURNVAL: // fp->var = regs[a]
            load_register(V0, single);
            m.v_store(V0, FP, slot_number(0));
            return emitted;
        case Opcode::MOV_ABS:   // (task->stack+a)->var = regs[EBX]
            m.p_load(PA, TASK, (int32_t) offsetof(fiber, stack));
            load_register(V0, EBX);
            m.v_store(V0, PA, slot_number(single));
            return emitted;
        case Opcode::LOAD_ABS:  // regs[EBX] = (task->stack+a)->var
            m.p_load(PA, TASK, (int32_t) offsetof(fiber, stack));
            m.v_load(V0, PA, slot_number(single));
            store_register(EBX, V0);
            return emitted;
        case Opcode::ADDL:      // (fp+b)->var += regs[a]
            local_operation(V_ADD, dual2, dual1);
            return emitted;
        case Opcode::SUBL:
            local_operation(V_SUB, dual2, dual1);
            return emitted;
        case Opcode::MULL:
            local_operation(V_MUL, dual2, dual1);
            return emitted;
        case Opcode::MODL:      // (fp+b)->var = vm_remainder(local, regs[a])
            m.v_load(V0, FP, slot_number(dual2));
            load_register(V1, dual1);
            remainder_into_v0();
            m.v_store(V0, FP, slot_number(dual2));
            return emitted;
        case Opcode::ANDL:
            local_operation(V_AND, dual2, dual1);
            return emitted;
        case Opcode::ORL:
            local_operation(V_OR, dual2, dual1);
            return emitted;
        case Opcode::XORL:
            local_operation(V_XOR, dual2, dual1);
            return emitted;
        case Opcode::IADDL:     // (fp+a)->var += b
            local_operation_with_constant(V_ADD, single, (int64_t) raw);
            return emitted;
        case Opcode::ISUBL:
            local_operation_with_constant(V_SUB, single, (int64_t) raw);
            return emitted;
        case Opcode::IMULL:
            local_operation_with_constant(V_MUL, single, (int64_t) raw);
            return emitted;
        case Opcode::IMODL:
            m.v_load(V0, FP, slot_number(single));
            m.v_constant(V1, (int64_t) raw);
            remainder_into_v0();
            m.v_store(V0, FP, slot_number(single));
            return emitted;
        case Opcode::DADDL:     // (fp+b)->var.d += regs[a].d
            local_decimal_operation(D_ADD, dual2, dual1);
            return emitted;
        case Opcode::DSUBL:
            local_decimal_operation(D_SUB, dual2, dual1);
            return emitted;
        case Opcode::DMULL:
            local_decimal_operation(D_MUL, dual2, dual1);
            return emitted;
        case Opcode::DDIVL:
            local_decimal_operation(D_DIV, dual2, dual1);
            return emitted;
        case Opcode::DIADDL:    // (fp+a)->var.d += (double)b
            local_decimal_operation_with_constant(D_ADD, single, (double) raw);
            return emitted;
        case Opcode::DISUBL:
            local_decimal_operation_with_constant(D_SUB, single, (double) raw);
            return emitted;
        case Opcode::DIMULL:
            local_decimal_operation_with_constant(D_MUL, single, (double) raw);
            return emitted;
        case Opcode::DIDIVL:
            local_decimal_operation_with_constant(D_DIV, single, (double) raw);
            return emitted;
        default:
            return not_mine;
    }
}

} // namespace armjit
