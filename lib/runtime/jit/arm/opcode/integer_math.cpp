//
// Whole-number instructions (jit/x64/opcode/integer_math.cpp): add sub mul mod, compares, bit
// operations, the narrowing moves mov8 .. movu64.
//
#include "../arm_generator.h"

namespace armjit {

emit_result ArmGenerator::emit_integer_math() {
    switch(op) {
        case Opcode::MOVI:      // regs[a] = b
            m.v_constant(V0, (int64_t) raw);
            store_register(single, V0);
            return emitted;
        case Opcode::MOVR:      // regs[a] = regs[b]   (integer or decimal: the cell is copied)
            load_register(V0, dual2);
            store_register(dual1, V0);
            return emitted;
        case Opcode::ADD:       // regs[c] = regs[a] + regs[b]
            integer_operation(V_ADD, triple1, triple2, triple3);
            return emitted;
        case Opcode::SUB:
            integer_operation(V_SUB, triple1, triple2, triple3);
            return emitted;
        case Opcode::MUL:
            integer_operation(V_MUL, triple1, triple2, triple3);
            return emitted;
        case Opcode::MOD:       // regs[c] = vm_remainder(regs[a], regs[b])   (raises on 0)
            load_register(V0, triple1);
            load_register(V1, triple2);
            remainder_into_v0();
            store_register(triple3, V0);
            return emitted;
        case Opcode::IADD:      // regs[a] += b
            integer_operation_with_constant(V_ADD, single, (int64_t) raw);
            return emitted;
        case Opcode::ISUB:
            integer_operation_with_constant(V_SUB, single, (int64_t) raw);
            return emitted;
        case Opcode::IMUL:
            integer_operation_with_constant(V_MUL, single, (int64_t) raw);
            return emitted;
        case Opcode::IMOD:
            load_register(V0, single);
            m.v_constant(V1, (int64_t) raw);
            remainder_into_v0();
            store_register(single, V0);
            return emitted;
        case Opcode::INC:
            integer_operation_with_constant(V_ADD, single, 1);
            return emitted;
        case Opcode::DEC:
            integer_operation_with_constant(V_SUB, single, 1);
            return emitted;
        case Opcode::NEG:       // regs[a] = -regs[b]
            load_register(V0, dual2);
            m.v_negate(V0);
            store_register(dual1, V0);
            return emitted;
        case Opcode::NOT:       // regs[a] = !regs[b]
            load_register(V0, dual2);
            m.v_constant(V1, 0);
            m.v_compare(V0, V0, V1, EQ);
            store_register(dual1, V0);
            return emitted;
        case Opcode::LT:        // regs[CMT] = regs[a] < regs[b]
            integer_compare(dual1, dual2, CMT, LT);
            return emitted;
        case Opcode::GT:
            integer_compare(dual1, dual2, CMT, GT);
            return emitted;
        case Opcode::LTE:
            integer_compare(dual1, dual2, CMT, LE);
            return emitted;
        case Opcode::GTE:
            integer_compare(dual1, dual2, CMT, GE);
            return emitted;
        case Opcode::TEST:      // regs[BMR] = regs[a] == regs[b]
            integer_compare(dual1, dual2, BMR, EQ);
            return emitted;
        case Opcode::TNE:
            integer_compare(dual1, dual2, BMR, NE);
            return emitted;
        case Opcode::CMP:       // regs[CMT] = regs[a] == b
            load_register(V0, single);
            m.v_constant(V1, (int64_t) raw);
            m.v_compare(V0, V0, V1, EQ);
            store_register(CMT, V0);
            return emitted;
        case Opcode::AND:       // regs[BMR] = regs[a] && regs[b]
            load_register(V0, dual1);
            load_register(V1, dual2);
            m.v_not_zero(V0);
            m.v_not_zero(V1);
            m.v_operation(V_AND, V0, V0, V1);
            store_register(BMR, V0);
            return emitted;
        case Opcode::UAND:      // regs[BMR] = regs[a] & regs[b]
            integer_operation(V_AND, dual1, dual2, BMR);
            return emitted;
        case Opcode::OR:
            integer_operation(V_OR, dual1, dual2, BMR);
            return emitted;
        case Opcode::XOR:
            integer_operation(V_XOR, dual1, dual2, BMR);
            return emitted;
        case Opcode::SHL:       // regs[c] = regs[a] << regs[b]   (the count is taken modulo 64, as the CPU of a PC does)
            integer_operation(V_SHL, triple1, triple2, triple3);
            return emitted;
        case Opcode::SHR:
            integer_operation(V_SAR, triple1, triple2, triple3);
            return emitted;
        case Opcode::MOV8: case Opcode::MOV16: case Opcode::MOV32:
        case Opcode::MOVU8: case Opcode::MOVU16: case Opcode::MOVU32:
            load_register(V0, dual2);
            m.v_extend(V0, op == Opcode::MOV8 || op == Opcode::MOVU8 ? 8 : op == Opcode::MOV16 || op == Opcode::MOVU16 ? 16 : 32,
                       op == Opcode::MOV8 || op == Opcode::MOV16 || op == Opcode::MOV32);
            store_register(dual1, V0);
            return emitted;
        case Opcode::MOV64:
        case Opcode::MOVU64:
            load_register(V0, dual2);
            store_register(dual1, V0);
            return emitted;
        default:
            return not_mine;
    }
}

} // namespace armjit
