//
// Sharp bytecode optimizer - values known at compile time
// =======================================================
// The machinery behind constant propagation (passes/pass_constant_propagation.cpp)
// and everything else that wants to know "what is in this register / local
// here?" (passes/pass_loop_unrolling.cpp runs whole loops with it).
//
// Every register and number local is in one of three states at each point:
//     unknown yet   nothing has been seen on any path so far
//     constant c    every path seen so far leaves the value c in it
//     varying       paths disagree, or the value is not known at compile time
//
// ONLY WHOLE NUMBERS THAT FIT IN 32 BITS ARE TRACKED
//   VM registers are `long double`, locals are `double`, and the compiler may
//   itself run on a machine with a different `long double`. A whole number of
//   that size is exact in all of them, so folding it at compile time gives
//   bit-for-bit what the VM would have computed. Fractions, huge values, and
//   anything that could become negative zero are left for run time.
//

#ifndef SHARP_OPT_KNOWN_VALUES_H
#define SHARP_OPT_KNOWN_VALUES_H

#include "control_flow.h"
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

inline known_value constant(int64_t value) {
    // outside the range we are willing to reason about
    if(value < INT32_MIN || value > INT32_MAX) return VARYING;
    return { state_constant, value };
}

inline known_value meet(const known_value &a, const known_value &b) {
    if(a.state == state_unknown) return b;
    if(b.state == state_unknown) return a;
    if(a.state == state_constant && b.state == state_constant && a.value == b.value) return a;
    return VARYING;
}

inline bool same_value(const known_value &a, const known_value &b) {
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

inline bool is_known(const known_value &v) { return v.state == state_constant; }

/**
 * Update `state` for one instruction. This is the single description of what
 * each opcode does to known values; every user of this file goes through it.
 */
void apply_instruction(const ir_instruction &instr, machine_state &state);

/**
 * The forward data flow analysis: in[b] = what is known when block b is entered.
 * Returns false (nothing may be concluded) if the function uses a register or
 * local slot number the analysis does not understand.
 */
bool analyze_known_values(function_ir &ir, control_flow &cfg, std::vector<machine_state> &in);

#endif //SHARP_OPT_KNOWN_VALUES_H
