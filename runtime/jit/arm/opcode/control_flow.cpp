//
// Jumps and branches (jit/x64/opcode/control_flow.cpp).
//
#include "../arm_generator.h"

namespace armjit {

emit_result ArmGenerator::emit_control_flow() {
    switch(op) {
        case Opcode::NOP:
            return emitted;
        case Opcode::JMP:       // goto a
            return jump_to_instruction(single) ? emitted : cannot_compile;
        case Opcode::JE: {      // if(regs[CMT]) goto a
            int skip = m.label();
            load_register(V0, CMT);
            m.v_jump_if_zero(V0, skip);
            if(!jump_to_instruction(single)) return cannot_compile;
            m.bind(skip);
            return emitted;
        }
        case Opcode::JNE: {     // if(!regs[CMT]) goto a
            int skip = m.label();
            load_register(V0, CMT);
            m.v_jump_if_not_zero(V0, skip);
            if(!jump_to_instruction(single)) return cannot_compile;
            m.bind(skip);
            return emitted;
        }
        case Opcode::SKIP:      // pc += a, then on to the next instruction
            if(next + single < 0 || next + single >= count || !starts[next + single]) return cannot_compile;
            m.jump(labels[next + single]);
            return emitted;
        case Opcode::SKPE:      // if(regs[a] != 0) pc += b;  (else the next instruction)
            if(pc + dual2 < 0 || pc + dual2 >= count || !starts[pc + dual2]) return cannot_compile;
            load_register(V0, dual1);
            m.v_jump_if_not_zero(V0, labels[pc + dual2]);
            return emitted;
        case Opcode::SKNE:      // if(regs[a] == 0) pc += b;
            if(pc + dual2 < 0 || pc + dual2 >= count || !starts[pc + dual2]) return cannot_compile;
            load_register(V0, dual1);
            m.v_jump_if_zero(V0, labels[pc + dual2]);
            return emitted;
        case Opcode::BRH:       // goto regs[ADX]
            load_register(V0, ADX);
            m.v_to_pointer(PA, V0);
            m.jump(dispatch);
            return emitted;
        case Opcode::IFE: {     // if(regs[CMT]) goto regs[ADX]
            int skip = m.label();
            load_register(V0, CMT);
            m.v_jump_if_zero(V0, skip);
            load_register(V0, ADX);
            m.v_to_pointer(PA, V0);
            m.jump(dispatch);
            m.bind(skip);
            return emitted;
        }
        case Opcode::IFNE: {
            int skip = m.label();
            load_register(V0, CMT);
            m.v_jump_if_not_zero(V0, skip);
            load_register(V0, ADX);
            m.v_to_pointer(PA, V0);
            m.jump(dispatch);
            m.bind(skip);
            return emitted;
        }
        case Opcode::LOADPC:    // regs[a] = this pc
            m.v_constant(V0, (int64_t) pc);
            store_register(single, V0);
            return emitted;
        default:
            return not_mine;
    }
}

} // namespace armjit
