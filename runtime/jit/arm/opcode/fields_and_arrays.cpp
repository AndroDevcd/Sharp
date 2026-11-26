//
// Fields of an object and items of a number array (jit/x64/opcode/fields_and_arrays.cpp).
//
#include "../arm_generator.h"

namespace armjit {

emit_result ArmGenerator::emit_fields_and_arrays() {
    switch(op) {
        case Opcode::GETF:      // regs[a] = ptr->o.<slot b>
            load_object_or_throw(PA);
            m.p_load(PA, PA, (int32_t) offsetof(sharp_object, node));
            m.v_load(V0, PA, (int32_t) (dual2 * 8));
            store_register(dual1, V0);
            return emitted;
        case Opcode::PUTF:      // ptr->o.<slot b> = regs[a]
            load_object_or_throw(PA);
            m.p_load(PA, PA, (int32_t) offsetof(sharp_object, node));
            load_register(V0, dual1);
            m.v_store(V0, PA, (int32_t) (dual2 * 8));
            return emitted;
        case Opcode::ALOAD8: case Opcode::ALOADU8: case Opcode::ALOAD16: case Opcode::ALOADU16:
        case Opcode::ALOAD32: case Opcode::ALOADU32: case Opcode::ALOAD64: {
            // regs[a] = ((T*)ptr->o->data)[regs[b]]     (CHECKLEN has looked at the index)
            int bytes = op == Opcode::ALOAD8 || op == Opcode::ALOADU8 ? 1 : op == Opcode::ALOAD16 || op == Opcode::ALOADU16 ? 2
                      : op == Opcode::ALOAD32 || op == Opcode::ALOADU32 ? 4 : 8;
            bool is_signed = op == Opcode::ALOAD8 || op == Opcode::ALOAD16 || op == Opcode::ALOAD32;
            load_object_or_throw(PA);
            m.p_load(PA, PA, (int32_t) offsetof(sharp_object, data));
            load_register(V0, dual2);
            m.v_to_pointer(PB, V0);
            m.item_load(V0, PA, PB, bytes, is_signed);
            store_register(dual1, V0);
            return emitted;
        }
        case Opcode::ASTORE8: case Opcode::ASTORE16: case Opcode::ASTORE32: case Opcode::ASTORE64: {
            // ((T*)ptr->o->data)[regs[a]] = regs[b]
            int bytes = op == Opcode::ASTORE8 ? 1 : op == Opcode::ASTORE16 ? 2 : op == Opcode::ASTORE32 ? 4 : 8;
            load_object_or_throw(PA);
            m.p_load(PA, PA, (int32_t) offsetof(sharp_object, data));
            load_register(V0, dual1);
            m.v_to_pointer(PB, V0);
            load_register(V0, dual2);
            m.item_store(V0, PA, PB, bytes);
            return emitted;
        }
        default:
            return not_mine;
    }
}

} // namespace armjit
