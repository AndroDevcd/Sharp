//
// THE SLOT FORMS (runtime/opcode/opcode.h, 0xa4 .. 0xf0; jit/x64/opcode/slot_forms.cpp): one
// instruction that reads its operands from frame slots and writes its result into one.
//
//      add_s d, a, b               V0 = fp[a];  V1 = fp[b];  V0 = V0 + V1;  fp[d] = V0
//      jlt_s a, b, sense -> L      V0 = fp[a];  V1 = fp[b];  if((V0 < V1) == sense) goto L
//      aload64_s d, array, index   ptr = &fp[array].obj (as `movl` would), null and bounds checked,
//                                  fp[d] = the element
//
// Written with the machine's operations, so it is the same for both ARM processors. Arithmetic and
// jumps are asked of the machine in one piece (slot_operation, slot_jump_if ...): one that keeps
// slots in registers (home_locals) computes in them; the rest goes through v_load / v_store.
//
#include "../arm_generator.h"

namespace armjit {

/** ptr on the object slot (as `movl` does); `to` = its object, or the null exception */
void ArmGenerator::slot_object_or_throw(Int slot, PReg to) {
    m.p_add(PTR, FP, slot_object(slot));
    load_object_or_throw(to);
}

/** ... and V1 = the index in slot `index`, inside the array or the bounds exception (PB is used) */
void ArmGenerator::slot_element_or_throw(Int array, Int index, PReg to) {
    slot_object_or_throw(array, to);
    int outside = request_slow_path(slow_bounds, pc);       // (it reads the index from V1)
    m.v_load(V1, FP, slot_number(index));
    m.p_load_32(PB, to, (int32_t) offsetof(sharp_object, size));
    m.v_jump_if_not_below(V1, PB, outside);                 // (a negative index is a huge one)
}

emit_result ArmGenerator::emit_slot_forms() {
    switch(op) {
        // ---- whole numbers: fp[a] = fp[b] op fp[c]
        case Opcode::ADD_S: case Opcode::SUB_S: case Opcode::MUL_S: case Opcode::AND_S:
        case Opcode::OR_S: case Opcode::XOR_S: case Opcode::SHL_S: case Opcode::SHR_S: {
            static const VOp operations[8] = { V_ADD, V_SUB, V_MUL, V_AND, V_OR, V_XOR, V_SHL, V_SAR };
            m.slot_operation(operations[op - Opcode::ADD_S], slot_number(triple1), slot_number(triple2), slot_number(triple3));
            return emitted;
        }

        // ---- decimals
        case Opcode::DADD_S: case Opcode::DSUB_S: case Opcode::DMUL_S: case Opcode::DDIV_S: {
            static const DOp operations[4] = { D_ADD, D_SUB, D_MUL, D_DIV };
            m.slot_decimal_operation(operations[op - Opcode::DADD_S], slot_number(triple1), slot_number(triple2), slot_number(triple3));
            return emitted;
        }
        case Opcode::DMATH_S:   // fp[a] = math function c of fp[b]
            m.d_load(D0, FP, slot_number(triple2));
            if(triple3 == 0xe) m.d_square_root(D0, D0);
            else if(triple3 == 0x11) m.d_absolute(D0, D0);
            else if((triple3 == 0x10 || triple3 == 0xf) && m.d_round(D0, D0, triple3 == 0xf)) { }
            else {
                m.d_constant(D1, 0);
                m.call(reinterpret_cast<void*>(jit_rt_math_numbered), { Arg::constant((int64_t) triple3), Arg::decimal(D0), Arg::decimal(D1) });
                m.result_to_decimal(D0);
            }
            m.d_store(D0, FP, slot_number(triple1));
            return emitted;
        case Opcode::I2D_S:
            m.slot_whole_to_decimal(slot_number(triple1), slot_number(triple2));
            return emitted;
        case Opcode::D2I_S:     // (the runtime's own conversion, as D2I)
            m.d_load(D0, FP, slot_number(triple2));
            m.call(reinterpret_cast<void*>(jit_rt_decimal_to_integer), { Arg::decimal(D0) });
            m.result_to_value(V0);
            m.v_store(V0, FP, slot_number(triple1));
            return emitted;

        // ---- compare and jump: if((fp[a] ? fp[b]) == c) goto <second word>
        case Opcode::JLT_S: case Opcode::JGT_S: case Opcode::JLE_S: case Opcode::JGE_S:
        case Opcode::JEQ_S: case Opcode::JNE_S:
        case Opcode::DJLT_S: case Opcode::DJGT_S: case Opcode::DJLE_S: case Opcode::DJGE_S:
        case Opcode::DJEQ_S: case Opcode::DJNE_S: {
            static const Cond conditions[6] = { LT, GT, LE, GE, EQ, NE };
            if(!before_conditional_jump(raw)) return cannot_compile;
            // jump when the comparison is what the instruction asks for
            if(op >= Opcode::DJLT_S)
                m.slot_decimal_jump_if(slot_number(triple1), slot_number(triple2), conditions[op - Opcode::DJLT_S], triple3 != 0, labels[raw]);
            else
                m.slot_jump_if(slot_number(triple1), slot_number(triple2), conditions[op - Opcode::JLT_S], triple3 != 0, labels[raw]);
            return emitted;
        }

        // ---- elements of a number array
        case Opcode::ALOAD8_S: case Opcode::ALOADU8_S: case Opcode::ALOAD16_S: case Opcode::ALOADU16_S:
        case Opcode::ALOAD32_S: case Opcode::ALOADU32_S: case Opcode::ALOAD64_S: {
            int bytes = op == Opcode::ALOAD8_S || op == Opcode::ALOADU8_S ? 1 : op == Opcode::ALOAD16_S || op == Opcode::ALOADU16_S ? 2
                      : op == Opcode::ALOAD32_S || op == Opcode::ALOADU32_S ? 4 : 8;
            bool is_signed = op == Opcode::ALOAD8_S || op == Opcode::ALOAD16_S || op == Opcode::ALOAD32_S;
            slot_element_or_throw(triple2, triple3, PA);
            m.p_load(PA, PA, (int32_t) offsetof(sharp_object, data));
            m.v_to_pointer(PB, V1);
            m.item_load(V0, PA, PB, bytes, is_signed);
            m.v_store(V0, FP, slot_number(triple1));
            return emitted;
        }
        case Opcode::ASTORE8_S: case Opcode::ASTORE16_S: case Opcode::ASTORE32_S: case Opcode::ASTORE64_S: {
            int bytes = op == Opcode::ASTORE8_S ? 1 : op == Opcode::ASTORE16_S ? 2 : op == Opcode::ASTORE32_S ? 4 : 8;
            slot_element_or_throw(triple1, triple2, PA);
            m.p_load(PA, PA, (int32_t) offsetof(sharp_object, data));
            m.v_to_pointer(PB, V1);
            m.v_load(V0, FP, slot_number(triple3));
            m.item_store(V0, PA, PB, bytes);
            return emitted;
        }

        // ---- number fields
        case Opcode::GETF_S:    // fp[a] = (fp[b].obj).<field c>
            slot_object_or_throw(triple2, PA);
            m.p_load(PA, PA, (int32_t) offsetof(sharp_object, node));
            m.v_load(V0, PA, (int32_t) (triple3 * 8));
            m.v_store(V0, FP, slot_number(triple1));
            return emitted;
        case Opcode::PUTF_S:    // (fp[a].obj).<field b> = fp[c]
            slot_object_or_throw(triple1, PA);
            m.p_load(PA, PA, (int32_t) offsetof(sharp_object, node));
            m.v_load(V0, FP, slot_number(triple3));
            m.v_store(V0, PA, (int32_t) (triple2 * 8));
            return emitted;
        case Opcode::SIZEOF_S: {    // fp[a] = the length of fp[b].obj, 0 for null
            int done = m.label();
            m.v_constant(V0, 0);
            m.p_load(PA, FP, slot_object(triple2));
            m.p_jump_if_zero(PA, done);
            m.p_load_32(PB, PA, (int32_t) offsetof(sharp_object, size));
            m.v_from_pointer(V0, PB);
            m.bind(done);
            m.v_store(V0, FP, slot_number(triple1));
            return emitted;
        }

        // ---- references out of an array and out of an object, into a slot (a stack slot is a root
        // of the collector: no write barrier, as PUSHL and POPL have none)
        case Opcode::AGETOBJ_S:
            slot_element_or_throw(triple2, triple3, PA);
            m.p_load(PA, PA, (int32_t) offsetof(sharp_object, node));
            m.v_to_pointer(PB, V1);
            m.p_add_scaled(PA, PA, PB, 3);
            m.p_load(PA, PA, 0);
            m.p_store_object(PA, FP, slot_object(triple1));
            return emitted;
        case Opcode::GETOBJ_S:
            slot_object_or_throw(triple2, PA);
            m.p_load(PA, PA, (int32_t) offsetof(sharp_object, node));
            m.p_load(PA, PA, (int32_t) (triple3 * 8));
            m.p_store_object(PA, FP, slot_object(triple1));
            return emitted;

        // ================================================================ the second group
        case Opcode::JNZ_S: case Opcode::JZ_S: {    // if(fp[a] != 0) / if(fp[a] == 0) goto <second word>
            if(!before_conditional_jump(raw)) return cannot_compile;
            m.slot_jump_if_zero(slot_number(single), op == Opcode::JZ_S, labels[raw]);
            return emitted;
        }
        case Opcode::JNULL_S: {     // if((fp[a].obj is null) == c) goto <second word>
            if(!before_conditional_jump(raw)) return cannot_compile;
            m.p_load(PA, FP, slot_object(triple1));
            if(triple3 != 0) m.p_jump_if_zero(PA, labels[raw]); else m.p_jump_if_not_zero(PA, labels[raw]);
            return emitted;
        }
        case Opcode::ISNULL_S: {    // fp[a] = fp[b].obj is null
            int done = m.label();
            m.v_constant(V0, 1);
            m.p_load(PA, FP, slot_object(triple2));
            m.p_jump_if_zero(PA, done);
            m.v_constant(V0, 0);
            m.bind(done);
            m.v_store(V0, FP, slot_number(triple1));
            return emitted;
        }
        case Opcode::NOT_S:         // fp[a] = !fp[b]
            m.v_load(V0, FP, slot_number(triple2));
            m.v_constant(V1, 0);
            m.v_compare(V0, V0, V1, EQ);
            m.v_store(V0, FP, slot_number(triple1));
            return emitted;
        case Opcode::MOV8_S: case Opcode::MOV16_S: case Opcode::MOV32_S:
        case Opcode::MOVU8_S: case Opcode::MOVU16_S: case Opcode::MOVU32_S:
            m.v_load(V0, FP, slot_number(triple2));
            m.v_extend(V0, op == Opcode::MOV8_S || op == Opcode::MOVU8_S ? 8 : op == Opcode::MOV16_S || op == Opcode::MOVU16_S ? 16 : 32,
                       op == Opcode::MOV8_S || op == Opcode::MOV16_S || op == Opcode::MOV32_S);
            m.v_store(V0, FP, slot_number(triple1));
            return emitted;
        case Opcode::RETURNVAL_S:   // fp->var = fp[a]
            m.v_load(V0, FP, slot_number(single));
            m.v_store(V0, FP, slot_number(0));
            return emitted;
        case Opcode::RETURNOBJ_S:   // fp->obj = fp[a].obj
            m.p_load(PA, FP, slot_object(single));
            m.p_store_object(PA, FP, slot_object(0));
            return emitted;
        case Opcode::IADD_S: case Opcode::ISUB_S: case Opcode::IMUL_S:     // fp[a] = fp[b] op <second word>
            m.slot_operation_constant(op == Opcode::IADD_S ? V_ADD : op == Opcode::ISUB_S ? V_SUB : V_MUL,
                                      slot_number(triple1), slot_number(triple2), (int64_t) raw);
            return emitted;
        case Opcode::PUTOBJ_S: {    // (fp[a].obj).<reference field b> = fp[c].obj: movl, movn and popobj together
            slot_object_or_throw(triple1, PA);                              // PA = the owner
            m.p_store(PA, TASK, (int32_t) offsetof(fiber, ptrOwner));
            m.p_load(PB, PA, (int32_t) offsetof(sharp_object, node));
            m.p_add(PTR, PB, (int32_t) (triple2 * 8));
            m.p_load(PA, FP, slot_object(triple3));                         // PA = the value
            m.p_store_object(PA, PTR, 0);
            // the write barrier, as POPOBJ (opcode/objects.cpp)
            int done = m.label();
            int remember = request_slow_path(slow_barrier, pc, done);
            m.p_jump_if_zero(PA, done);
            m.p_load_32(PB, PA, (int32_t) offsetof(sharp_object, info));
            m.p_jump_if_bits(PB, 0x30000000, true, done);       // the value is not young
            m.p_load(PA, TASK, (int32_t) offsetof(fiber, ptrOwner));
            m.p_load_32(PB, PA, (int32_t) offsetof(sharp_object, info));
            m.p_jump_if_bits(PB, 0x30000000, false, done);      // the owner is young
            m.p_load_8(PB, PA, (int32_t) offsetof(sharp_object, remembered));
            m.p_jump_if_zero(PB, remember);
            m.bind(done);
            return emitted;
        }
        case Opcode::MOVOBJ_S:      // fp[a].obj = fp[b].obj
            m.p_load(PA, FP, slot_object(triple2));
            m.p_store_object(PA, FP, slot_object(triple1));
            return emitted;

        // ================================================================ the third group: a constant operand
        case Opcode::IAND_S: case Opcode::IOR_S: case Opcode::IXOR_S: case Opcode::ISHL_S: case Opcode::ISHR_S: {
            static const VOp operations[5] = { V_AND, V_OR, V_XOR, V_SHL, V_SAR };
            m.slot_operation_constant(operations[op - Opcode::IAND_S], slot_number(triple1), slot_number(triple2), (int64_t) raw);
            return emitted;
        }
        case Opcode::JLT_K: case Opcode::JGT_K: case Opcode::JLE_K: case Opcode::JGE_K:
        case Opcode::JEQ_K: case Opcode::JNE_K:
        case Opcode::DJLT_K: case Opcode::DJGT_K: case Opcode::DJLE_K: case Opcode::DJGE_K:
        case Opcode::DJEQ_K: case Opcode::DJNE_K: {    // if((fp[a] ? b) == c) goto <second word>
            static const Cond conditions[6] = { LT, GT, LE, GE, EQ, NE };
            if(!before_conditional_jump(raw)) return cannot_compile;
            if(op >= Opcode::DJLT_K)
                m.slot_decimal_jump_if_constant(slot_number(triple1), (double) triple2, conditions[op - Opcode::DJLT_K], triple3 != 0, labels[raw]);
            else
                m.slot_jump_if_constant(slot_number(triple1), (int64_t) triple2, conditions[op - Opcode::JLT_K], triple3 != 0, labels[raw]);
            return emitted;
        }
        default:
            return not_mine;
    }
}

} // namespace armjit
