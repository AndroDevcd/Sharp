//
// Pass: constant propagation and folding   (-O2)
// ==============================================
// Works out, for every point in a function, which registers and number locals
// are certain to hold one particular known value there, and then:
//
//   * replaces a computation whose result is known with the result
//         movi 4, ecx ; movi 6, cx ; add cx, ecx -> bmr      ->   movi 10, bmr
//   * replaces a read of a known local with the value
//         istorel fp+2, 7 ... loadl ecx, fp+2                 ->   movi 7, ecx
//   * decides conditional jumps whose condition is known
//         movi 1, cmt ; jne @else         -> (jump removed, `else` side becomes unreachable)
//         movi 0, cmt ; jne @else         -> jmp @else
//
// Together with inlining this is what evaluates a pure function at compile
// time: after `square(5)` is inlined its body is `5 * 5` with nothing unknown
// in it, so it folds to 25. If only some inputs are known, the parts that
// depend on them fold and the rest stays as code.
//
// The instructions that fed a folded value are not deleted here; they simply no
// longer have a reader, and the dead code passes remove them.
//
// HOW (a standard forward data flow analysis)
//   Every register / local is in one of three states at each point:
//       unknown yet   nothing has been seen on any path so far
//       constant c    every path seen so far leaves the value c in it
//       varying       paths disagree, or the value is not known at compile time
//   Walking forward, each instruction updates the states it writes. Where
//   control flow joins, a value stays constant only if all incoming paths agree.
//   Loops are handled by repeating until the states stop changing (a loop
//   counter becomes "varying" the second time round).
//
// ONLY WHOLE NUMBERS THAT FIT IN 32 BITS ARE TRACKED
//   VM registers are `long double`, locals are `double`, and the compiler may
//   itself run on a machine with a different `long double`. A whole number of
//   that size is exact in all of them, so folding it at compile time gives
//   bit-for-bit what the VM would have computed. Fractions, huge values, and
//   anything that could become negative zero are left for run time.
//

#include "passes.h"
#include "../analysis/liveness.h"
#include <cstdint>

// ---------------------------------------------------------------------------------
// the three states
// ---------------------------------------------------------------------------------
enum value_state : uint8_t { state_unknown, state_constant, state_varying };

struct known_value {
    value_state state;
    int64_t value;
};

static const known_value VARYING = { state_varying, 0 };

static known_value constant(int64_t value) {
    // outside the range we are willing to reason about
    if(value < INT32_MIN || value > INT32_MAX) return VARYING;
    return { state_constant, value };
}

static known_value meet(const known_value &a, const known_value &b) {
    if(a.state == state_unknown) return b;
    if(b.state == state_unknown) return a;
    if(a.state == state_constant && b.state == state_constant && a.value == b.value) return a;
    return VARYING;
}

static bool same_value(const known_value &a, const known_value &b) {
    return a.state == b.state && (a.state != state_constant || a.value == b.value);
}

struct machine_state {
    known_value registers[10];
    std::vector<known_value> locals;

    void set_all(const known_value &value) {
        for(auto &reg : registers) reg = value;
        for(auto &local : locals) local = value;
    }

    bool merge(const machine_state &other) { // returns true if this changed
        bool changed = false;
        for(int r = 0; r < 10; r++) {
            known_value merged = meet(registers[r], other.registers[r]);
            if(!same_value(merged, registers[r])) { registers[r] = merged; changed = true; }
        }
        for(size_t i = 0; i < locals.size(); i++) {
            known_value merged = meet(locals[i], other.locals[i]);
            if(!same_value(merged, locals[i])) { locals[i] = merged; changed = true; }
        }
        return changed;
    }
};

// ---------------------------------------------------------------------------------
// folding
// ---------------------------------------------------------------------------------

static bool is_known(const known_value &v) { return v.state == state_constant; }

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

/**
 * Update `state` for one instruction. This is the single description of what
 * each opcode does to known values; the analysis and the rewrite both use it.
 */
static void apply_instruction(const ir_instruction &instr, machine_state &state) {
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
// the pass
// ---------------------------------------------------------------------------------

/** the one register a plain computation writes, or -1 */
static int single_register_written(const ir_instruction &instr) {
    register_set reads, writes;
    register_effects(instr, reads, writes);
    if(writes == 0 || (writes & (writes - 1)) != 0) return -1;

    for(int r = 0; r < 10; r++) {
        if((writes >> r) & 1u) return r;
    }
    return -1;
}

/** the one number local an instruction writes, or -1 */
static int local_written(const ir_instruction &instr) {
    const instruction_info &info = instruction_info_of(instr.op);
    if(info.a == k_var_w || info.a == k_var_rw) return instr.a;
    if(info.b == k_var_w || info.b == k_var_rw) return instr.b;
    return -1;
}

static void become(ir_instruction &instr, uint8_t op, int32_t a, int32_t b) {
    instr.op = op;
    instr.a = a; instr.b = b; instr.c = 0;
    instr.target = IR_NONE;
    instr.pcConstant = false;
    instr.modified = true;
}

bool pass_constant_propagation(function_ir &ir) {
    control_flow cfg;
    build_control_flow(ir, cfg);

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

    std::vector<machine_state> in(blocks, blank), out(blocks, blank);

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

    // ---- rewrite
    bool rewrote = false;

    for(size_t b = 0; b < blocks; b++) {
        if(!cfg.blocks[b].reachable) continue;
        machine_state state = in[b];

        for(int i = cfg.blocks[b].first; i <= cfg.blocks[b].last; i++) {
            ir_instruction &instr = ir.code[i];
            machine_state before = state;
            apply_instruction(instr, state);

            // -- a conditional jump on a known condition
            if(instr.op == Opcode::JE || instr.op == Opcode::JNE) {
                if(!is_known(before.registers[CMT])) continue;

                bool taken = (before.registers[CMT].value != 0) == (instr.op == Opcode::JE);
                if(taken) {
                    int target = instr.target;
                    become(instr, Opcode::JMP, 0, 0);
                    instr.target = target;
                } else {
                    become(instr, Opcode::NOP, 0, 0); // never taken; the dead code passes drop the nop
                }
                rewrote = true;
                continue;
            }

            // -- a pushed value that is known: push the constant itself
            if(instr.op == Opcode::RSTORE && is_known(before.registers[instr.a])) {
                become(instr, Opcode::ISTORE, (int32_t)before.registers[instr.a].value, 0);
                rewrote = true;
                continue;
            }
            if(instr.op == Opcode::IPUSHL && is_known(before.locals[instr.a])) {
                become(instr, Opcode::ISTORE, (int32_t)before.locals[instr.a].value, 0);
                rewrote = true;
                continue;
            }

            if(!is_plain_computation(instr)) continue;

            // -- a computation into a register whose result is known
            int reg = single_register_written(instr);
            int local = local_written(instr);

            if(reg >= 0 && local < 0 && is_known(state.registers[reg])) {
                if(instr.op == Opcode::MOVI && !instr.pcConstant) continue; // already the simplest form
                become(instr, Opcode::MOVI, reg, (int32_t)state.registers[reg].value);
                rewrote = true;
                continue;
            }

            // -- a computation into a local whose result is known
            if(local >= 0 && reg < 0 && is_known(state.locals[local])) {
                if(instr.op == Opcode::ISTOREL) continue;
                become(instr, Opcode::ISTOREL, local, (int32_t)state.locals[local].value);
                rewrote = true;
                continue;
            }
        }
    }

    return rewrote;
}
