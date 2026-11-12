//
// Sharp bytecode optimizer - values known at compile time
// See known_values.h.
//

#include "known_values.h"
#include "liveness.h"

// ---------------------------------------------------------------------------------
// folding
// ---------------------------------------------------------------------------------


/** left <op> right for the arithmetic opcodes, VARYING if it must be left to run time */
static known_value fold_arithmetic(uint8_t op, const known_value &left, const known_value &right) {
    if(!is_known(left) || !is_known(right)) return VARYING;
    int64_t a = left.value, b = right.value;

    switch(op) {
        case Opcode::ADD: case Opcode::IADD: case Opcode::ADDL: case Opcode::IADDL:
            return constant(a + b);
        case Opcode::SUB: case Opcode::ISUB: case Opcode::SUBL: case Opcode::ISUBL:
            return constant(a - b);
        case Opcode::MUL: case Opcode::IMUL: case Opcode::MULL: case Opcode::IMULL:
            if(a * b == 0 && (a < 0 || b < 0)) return VARYING; // floating point would give -0
            return constant(a * b);
        case Opcode::DIV: case Opcode::IDIV: case Opcode::DIVL: case Opcode::IDIVL:
            if(b == 0 || a % b != 0) return VARYING;           // infinity or a fraction
            if(a == 0 && b < 0) return VARYING;                // -0
            return constant(a / b);
        case Opcode::MOD: case Opcode::IMOD: case Opcode::MODL: case Opcode::IMODL:
            if(b == 0) return VARYING;                         // traps at run time, keep it
            return constant(a % b);
        case Opcode::SHL:
            if(b < 0 || b > 30) return VARYING;
            return constant(a * ((int64_t)1 << b));
        case Opcode::SHR:
            if(b < 0 || b > 62) return VARYING;
            return constant(a >> b);
        case Opcode::UAND: case Opcode::ANDL: return constant(a & b);
        case Opcode::OR:   case Opcode::ORL:  return constant(a | b);
        case Opcode::XOR:  case Opcode::XORL: return constant(a ^ b);
        case Opcode::AND:  return constant((a != 0 && b != 0) ? 1 : 0);
        case Opcode::LT:   return constant(a < b ? 1 : 0);
        case Opcode::GT:   return constant(a > b ? 1 : 0);
        case Opcode::LTE:  return constant(a <= b ? 1 : 0);
        case Opcode::GTE:  return constant(a >= b ? 1 : 0);
        case Opcode::TEST: case Opcode::CMP: return constant(a == b ? 1 : 0);
        case Opcode::TNE:  return constant(a != b ? 1 : 0);
        default:           return VARYING;
    }
}

static known_value fold_cast(uint8_t op, const known_value &v) {
    if(!is_known(v)) return VARYING;

    // Only when the value already fits the target type, so the cast changes
    // nothing. What a cast does to a number that does NOT fit is up to the
    // machine the VM runs on, so that is left for run time.
    int64_t value = v.value;
    switch(op) {
        case Opcode::MOV8:   return value >= INT8_MIN && value <= INT8_MAX ? v : VARYING;
        case Opcode::MOV16:  return value >= INT16_MIN && value <= INT16_MAX ? v : VARYING;
        case Opcode::MOV32:  return v;
        case Opcode::MOV64:  return v;
        case Opcode::MOVU8:  return value >= 0 && value <= UINT8_MAX ? v : VARYING;
        case Opcode::MOVU16: return value >= 0 && value <= UINT16_MAX ? v : VARYING;
        case Opcode::MOVU32: return value >= 0 ? v : VARYING;
        case Opcode::MOVU64: return value >= 0 ? v : VARYING;
        default:             return VARYING;
    }
}

void apply_instruction(const ir_instruction &instr, machine_state &state) {
    const instruction_info &info = instruction_info_of(instr.op);
    known_value *reg = state.registers;
    std::vector<known_value> &local = state.locals;

    switch(instr.op) {
        // ---- registers
        case Opcode::MOVI:
            reg[instr.a] = instr.pcConstant ? VARYING : constant(instr.b); // a code address is not a number we fold
            return;
        case Opcode::MOVR: reg[instr.a] = reg[instr.b]; return;
        case Opcode::ADD: case Opcode::SUB: case Opcode::MUL: case Opcode::DIV: case Opcode::MOD:
        case Opcode::SHL: case Opcode::SHR:
            reg[instr.c] = fold_arithmetic(instr.op, reg[instr.a], reg[instr.b]);
            return;
        case Opcode::IADD: case Opcode::ISUB: case Opcode::IMUL: case Opcode::IDIV: case Opcode::IMOD:
            reg[instr.a] = fold_arithmetic(instr.op, reg[instr.a], constant(instr.b));
            return;
        case Opcode::INC: reg[instr.a] = fold_arithmetic(Opcode::ADD, reg[instr.a], constant(1)); return;
        case Opcode::DEC: reg[instr.a] = fold_arithmetic(Opcode::SUB, reg[instr.a], constant(1)); return;
        case Opcode::NEG:
            reg[instr.a] = is_known(reg[instr.b]) && reg[instr.b].value != 0 ? constant(-reg[instr.b].value) : VARYING;
            return;
        case Opcode::NOT:
            reg[instr.a] = is_known(reg[instr.b]) ? constant(reg[instr.b].value == 0 ? 1 : 0) : VARYING;
            return;
        case Opcode::MOV8: case Opcode::MOV16: case Opcode::MOV32: case Opcode::MOV64:
        case Opcode::MOVU8: case Opcode::MOVU16: case Opcode::MOVU32: case Opcode::MOVU64:
            reg[instr.a] = fold_cast(instr.op, reg[instr.b]);
            return;
        case Opcode::LT: case Opcode::GT: case Opcode::LTE: case Opcode::GTE:
            reg[CMT] = fold_arithmetic(instr.op, reg[instr.a], reg[instr.b]);
            return;
        case Opcode::CMP:
            reg[CMT] = fold_arithmetic(instr.op, reg[instr.a], constant(instr.b));
            return;
        case Opcode::TEST: case Opcode::TNE: case Opcode::AND: case Opcode::UAND: case Opcode::OR: case Opcode::XOR:
            reg[BMR] = fold_arithmetic(instr.op, reg[instr.a], reg[instr.b]);
            return;

        // ---- locals
        case Opcode::LOADL:   reg[instr.a] = local[instr.b]; return;
        case Opcode::SMOVR_2: local[instr.b] = reg[instr.a]; return;
        case Opcode::ISTOREL: local[instr.a] = constant(instr.b); return;
        case Opcode::SMOVR_4: local[instr.a] = local[instr.b]; return;
        case Opcode::ADDL: case Opcode::SUBL: case Opcode::MULL: case Opcode::DIVL: case Opcode::MODL:
        case Opcode::ANDL: case Opcode::ORL: case Opcode::XORL:
            local[instr.b] = fold_arithmetic(instr.op, local[instr.b], reg[instr.a]);
            return;
        case Opcode::IADDL: case Opcode::ISUBL: case Opcode::IMULL: case Opcode::IDIVL: case Opcode::IMODL:
            local[instr.a] = fold_arithmetic(instr.op, local[instr.a], constant(instr.b));
            return;

        default:
            break;
    }

    // ---- anything else: whatever it writes is no longer known
    if(info.flags & f_opaque) {            // an interrupt may change anything
        state.set_all(VARYING);
        return;
    }

    if(info.flags & f_call) {              // the callee uses the same registers
        for(int r = 0; r < 10; r++) reg[r] = VARYING;
        return;
    }

    register_set reads, writes;
    register_effects(instr, reads, writes);
    for(int r = 0; r < 10; r++) {
        if((writes >> r) & 1u) reg[r] = VARYING;
    }

    const operand_kind kinds[3] = { info.a, info.b, info.c };
    const int32_t values[3] = { instr.a, instr.b, instr.c };
    for(int k = 0; k < 3; k++) {
        if(kinds[k] == k_var_w || kinds[k] == k_var_rw) local[values[k]] = VARYING;
    }
}

// ---------------------------------------------------------------------------------
// the analysis
// ---------------------------------------------------------------------------------

bool analyze_known_values(function_ir &ir, control_flow &cfg, std::vector<machine_state> &in) {
    // ---- locals must all be ones we understand
    int slots = ir.frameSlots > 0 ? ir.frameSlots : 1;
    for(ir_instruction &instr : ir.code) {
        const instruction_info &info = instruction_info_of(instr.op);
        const operand_kind kinds[3] = { info.a, info.b, info.c };
        const int32_t values[3] = { instr.a, instr.b, instr.c };
        for(int k = 0; k < 3; k++) {
            if(is_local_kind(kinds[k]) && (values[k] < 0 || values[k] >= slots)) return false;
            if(is_register_kind(kinds[k]) && (values[k] < 0 || values[k] > 9)) return false;
        }
    }

    size_t blocks = cfg.blocks.size();
    machine_state blank;
    blank.locals.assign(slots, { state_unknown, 0 });
    blank.set_all({ state_unknown, 0 });

    std::vector<machine_state> out(blocks, blank);
    in.assign(blocks, blank);

    // Nothing is known on entry (registers and locals hold leftovers), and nothing
    // is known when an exception handler is entered (it can be reached from the
    // middle of any protected instruction).
    auto entry_state = [&](size_t b) { return b == 0 || cfg.blocks[b].isHandler; };
    for(size_t b = 0; b < blocks; b++) {
        if(entry_state(b)) in[b].set_all(VARYING);
    }

    // ---- analysis: repeat until the states stop changing
    bool changed = true;
    while(changed) {
        changed = false;

        for(size_t b = 0; b < blocks; b++) {
            if(!cfg.blocks[b].reachable) continue;

            if(!entry_state(b)) {
                for(int pred : cfg.blocks[b].predecessors) {
                    if(cfg.blocks[pred].reachable) in[b].merge(out[pred]);
                }
            }

            machine_state state = in[b];
            for(int i = cfg.blocks[b].first; i <= cfg.blocks[b].last; i++)
                apply_instruction(ir.code[i], state);

            if(out[b].merge(state)) changed = true;
        }
    }

    return true;
}
