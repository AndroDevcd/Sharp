//
// Decimal instructions (jit/x64/opcode/decimal_math.cpp): dadd dsub dmul ddiv dexp, compares,
// i2d / d2i, constants.
//
#include "../arm_generator.h"

namespace armjit {

emit_result ArmGenerator::emit_decimal_math() {
    switch(op) {
        case Opcode::DMOVI:     // regs[a] = (double)b
            m.v_constant(V0, bits_of((double) raw));
            store_register(single, V0);
            return emitted;
        case Opcode::LDC:       // regs[a] = vm.constants[b]
            m.v_constant(V0, bits_of(vm.constants[dual2]));
            store_register(dual1, V0);
            return emitted;
        case Opcode::I2D:       // regs[a] = (double)regs[b]
            load_register(V0, dual2);
            m.d_from_whole(D0, V0);
            store_decimal(dual1, D0);
            return emitted;
        case Opcode::D2I:       // regs[a] = decimal_to_integer(regs[b])
            // (the runtime's own conversion, for every value: what it does with a decimal no whole
            //  number can hold is then the same on every processor)
            load_decimal(D0, dual2);
            m.call(reinterpret_cast<void*>(jit_rt_decimal_to_integer), { Arg::decimal(D0) });
            m.result_to_value(V0);
            store_register(dual1, V0);
            return emitted;
        case Opcode::DADD:      // regs[c] = regs[a] + regs[b]
            decimal_operation(D_ADD, triple1, triple2, triple3);
            return emitted;
        case Opcode::DSUB:
            decimal_operation(D_SUB, triple1, triple2, triple3);
            return emitted;
        case Opcode::DMUL:
            decimal_operation(D_MUL, triple1, triple2, triple3);
            return emitted;
        case Opcode::DDIV:      // decimals only: 7 / 2 is 3.5
            decimal_operation(D_DIV, triple1, triple2, triple3);
            return emitted;
        case Opcode::DEXP:      // regs[c] = pow(regs[a], regs[b])
            load_decimal(D0, triple1);
            load_decimal(D1, triple2);
            m.call(reinterpret_cast<void*>(jit_rt_power), { Arg::decimal(D0), Arg::decimal(D1) });
            m.result_to_decimal(D0);
            store_decimal(triple3, D0);
            return emitted;
        case Opcode::DSQRT:     // regs[a] = sqrt(regs[b]): the processor's own
            load_decimal(D0, dual2);
            m.d_square_root(D0, D0);
            store_decimal(dual1, D0);
            return emitted;
        case Opcode::DFLOOR: case Opcode::DCEIL:    // regs[a] = floor / ceil of regs[b]
            load_decimal(D0, dual2);
            if(!m.d_round(D0, D0, op == Opcode::DCEIL)) {
                // (a processor without the instruction - 32-bit ARM before ARMv8: the C library's)
                m.call(op == Opcode::DFLOOR ? reinterpret_cast<void*>(jit_rt_floor) : reinterpret_cast<void*>(jit_rt_ceil),
                       { Arg::decimal(D0) });
                m.result_to_decimal(D0);
            }
            store_decimal(dual1, D0);
            return emitted;
        case Opcode::DABS:      // regs[a] = regs[b] without its sign
            load_decimal(D0, dual2);
            m.d_absolute(D0, D0);
            store_decimal(dual1, D0);
            return emitted;
        case Opcode::DMATH:     // regs[a] = function c of regs[b] (and regs[EGX])
            load_decimal(D0, triple2);
            load_decimal(D1, EGX);
            m.call(reinterpret_cast<void*>(jit_rt_math_numbered), { Arg::constant((int64_t) triple3), Arg::decimal(D0), Arg::decimal(D1) });
            m.result_to_decimal(D0);
            store_decimal(triple1, D0);
            return emitted;
        case Opcode::DNEG:      // regs[a] = -regs[b]: flip the sign bit
            load_register(V0, dual2);
            m.v_flip_sign(V0);
            store_register(dual1, V0);
            return emitted;
        case Opcode::DINC:
            decimal_operation_with_constant(D_ADD, single, 1.0);
            return emitted;
        case Opcode::DDEC:
            decimal_operation_with_constant(D_SUB, single, 1.0);
            return emitted;
        case Opcode::DIADD:     // regs[a] += (double)b
            decimal_operation_with_constant(D_ADD, single, (double) raw);
            return emitted;
        case Opcode::DISUB:
            decimal_operation_with_constant(D_SUB, single, (double) raw);
            return emitted;
        case Opcode::DIMUL:
            decimal_operation_with_constant(D_MUL, single, (double) raw);
            return emitted;
        case Opcode::DIDIV:     // regs[a] /= (double)b
            decimal_operation_with_constant(D_DIV, single, (double) raw);
            return emitted;
        case Opcode::DLT:       // regs[CMT] = regs[a] < regs[b]
            decimal_compare(dual1, dual2, CMT, LT);
            return emitted;
        case Opcode::DGT:
            decimal_compare(dual1, dual2, CMT, GT);
            return emitted;
        case Opcode::DLTE:
            decimal_compare(dual1, dual2, CMT, LE);
            return emitted;
        case Opcode::DGTE:
            decimal_compare(dual1, dual2, CMT, GE);
            return emitted;
        case Opcode::DTEST:     // regs[BMR] = regs[a] == regs[b]
            decimal_compare(dual1, dual2, BMR, EQ);
            return emitted;
        case Opcode::DTNE:
            decimal_compare(dual1, dual2, BMR, NE);
            return emitted;
        case Opcode::DCMP:      // regs[CMT] = regs[a] == (double)b
            load_decimal(D0, single);
            m.d_constant(D1, (double) raw);
            m.d_compare(V0, D0, D1, EQ);
            store_register(CMT, V0);
            return emitted;
        case Opcode::DNOT:      // regs[a] = regs[b] == 0.0
            load_decimal(D0, dual2);
            m.d_constant(D1, 0.0);
            m.d_compare(V0, D0, D1, EQ);
            store_register(dual1, V0);
            return emitted;
        default:
            return not_mine;
    }
}

} // namespace armjit
