//
// Sharp bytecode optimizer - liveness analysis
// ============================================
//
// Answers one question for every instruction: "after this instruction runs,
// which values may still be read before they are overwritten?" A value that
// will not be read again is DEAD, and code whose only purpose is to produce a
// dead value can be removed.
//
// Two kinds of values are tracked:
//
//   * the ten VM registers
//   * the NUMBER part of every local slot ((fp+n)->var): parameters, local
//     variables and the compiler's spill slots
//
// The object part of a slot is not tracked (objects can be reached through
// task->ptr and from other threads, so "not read again" is not something the
// optimizer can know).
//
// WHAT IS ASSUMED AT THE EDGES OF A FUNCTION
//   - registers hold nothing meaningful when a function starts, and nothing
//     reads them after it returns. Generated code passes arguments and results
//     on the stack. (Hand written asm may break this rule, which is one reason
//     functions containing asm are never optimized.)
//   - a call does NOT end the life of a register. The callee may overwrite any
//     register, but generated code occasionally keeps a value in one across a
//     call to a function that does not touch it, so a register read after a
//     call counts as needed before the call.
//   - `int` (interrupts) may read every register and every local.
//   - when the function returns only slot 0's number is still needed: it is
//     the return value slot (RETURNVAL writes it, the caller reads it).
//
// EXCEPTIONS
//   An instruction inside a try block may jump to a handler before it finishes,
//   so everything the handlers need is treated as needed at every such
//   instruction.
//

#ifndef SHARP_OPT_LIVENESS_H
#define SHARP_OPT_LIVENESS_H

#include "control_flow.h"

/** A set of local slot numbers */
struct slot_set {
    std::vector<uint64_t> bits;

    void resize(int slots) { bits.assign((slots + 63) / 64, 0); }
    bool has(int slot) const { return (bits[slot >> 6] >> (slot & 63)) & 1u; }
    void add(int slot) { bits[slot >> 6] |= (uint64_t)1 << (slot & 63); }
    void remove(int slot) { bits[slot >> 6] &= ~((uint64_t)1 << (slot & 63)); }
    void add_all() { for(auto &word : bits) word = ~(uint64_t)0; }
    void clear() { for(auto &word : bits) word = 0; }
    bool merge(const slot_set &other) { // returns true if anything was added
        bool changed = false;
        for(size_t i = 0; i < bits.size(); i++) {
            uint64_t merged = bits[i] | other.bits[i];
            if(merged != bits[i]) { bits[i] = merged; changed = true; }
        }
        return changed;
    }
};

struct liveness {
    /** registers that may still be read after ir.code[i] has run */
    std::vector<register_set> registersAfter;

    /**
     * False if the function uses a local slot number the analysis does not
     * understand (outside 0..slotCount-1). Then nothing may be concluded about
     * locals and `localsAfter` is empty.
     */
    bool localsKnown;
    int slotCount;

    /** local slots whose number part may still be read after ir.code[i] has run */
    std::vector<slot_set> localsAfter;

    bool register_live_after(size_t index, int reg) const { return (registersAfter[index] >> reg) & 1u; }
    bool local_live_after(size_t index, int slot) const { return localsAfter[index].has(slot); }
};

/**
 * essential == nullptr: ordinary liveness, every read keeps its value alive.
 *
 * essential != nullptr ("strong" liveness, used by pass_dead_code.cpp):
 * essential[i] says instruction i stays in the program no matter what. A read
 * by any OTHER instruction only counts if that instruction's own result is
 * needed. This is what recognises a variable that is only used to compute its
 * own next value as dead.
 */
void compute_liveness(function_ir &ir, control_flow &cfg, liveness &live, const std::vector<char> *essential = nullptr);

/**
 * True if the instruction only computes: it writes registers and/or the number
 * part of local slots, has no other effect and cannot raise an exception. Such
 * an instruction can be deleted when nothing needs what it writes.
 */
bool is_plain_computation(const ir_instruction &instr);

/** Registers an instruction reads / writes (operands and implicit ones) */
void register_effects(const ir_instruction &instr, register_set &reads, register_set &writes);

#endif //SHARP_OPT_LIVENESS_H
