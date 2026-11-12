//
// Pass: loop variables in registers   (-O2, run after loop rotation)
// =================================================================
// Inside a loop, the number locals the loop works with are kept in VM registers
// instead of being loaded from and stored to their frame slot around every use
// ("register promotion").
//
//     for(i = 0; i < n; i++) { total += i; }
//
//     before (9 instructions per trip)       after (4 per trip, once copy
//                                             propagation has cleaned up)
//     -----------------------------------    -----------------------------------
//                                                    loadl ecf, i      ; once,
//                                                    loadl edf, total  ; before
//                                                    loadl ehf, n      ; the loop
//       body: loadl ecx, i                     body: add edf, ecf -> edf
//             loadl cx, total                        iadd ecf, 1
//             add cx, ecx -> bmr                     lt ecf, ehf
//             smovr_2 bmr, total                     je body
//             iaddl 1, i                             smovr_2 ecf, i    ; once,
//             loadl cx, i                            smovr_2 edf, total; after
//             loadl ecx, n
//             lt cx, ecx
//             je body
//
//   A value that never changes in the loop (`n`) is loaded once instead of on
//   every trip, which is what "moving invariant code out of the loop" gives.
//   Constants the loop uses (`i < 1000`) get the registers that are left over,
//   for the same reason.
//
// HOW
//   1. Find a register for each local: a register is free for a loop if no
//      instruction of the loop reads or writes it and nothing after the loop
//      still needs the value it held before.
//   2. In front of the loop: load each chosen local into its register.
//   3. In the loop: every access to the local becomes the same operation on
//      the register (table in translate()).
//   4. Behind the loop: store the registers of locals the loop changed.
//   This pass does the 1:1 replacement only, e.g. `loadl cx, i` becomes
//   `movr cx, ecf`. The copy propagation pass then removes those moves.
//
// WHICH LOOPS
//   The shape loop rotation leaves behind, and nothing fancy:
//   - entered at one place by running into it (so step 2 has a place to go)
//   - left at one place: the test at the bottom (so step 4 has a place to go).
//     Loops with `break` / `return` in the middle are left alone for now.
//   - no calls and no interrupts inside. A callee is free to overwrite every
//     register, and a call may switch to another fiber.
//   - no try/catch/finally in the function
//
// WHY IT IS SAFE
//   The number part of a frame slot can only be reached through the
//   instructions listed in translate(); if the loop touches a local in any
//   other way, that local is not promoted. Registers belong to the fiber and
//   are saved on a context switch like the frame is. If the loop is left by an
//   exception, the function is left with it (no try blocks here), and its
//   locals are gone.
//
// PRECISION: WHICH LOCALS MAY BE PROMOTED
//   A VM register is a `long double` (64 bits of precision on x86), a frame
//   slot is a `double` (53 bits). Storing a value into a local therefore ROUNDS
//   it, and a loop that stores on every trip rounds on every trip. Kept in a
//   register instead, the value would not be rounded until the loop ends, and
//   the result could differ in its last digits (`x = x * 1.1`), or once a whole
//   number grows past 2^53 (`x = x * 2 + i`). So a local is promoted only if
//   that cannot happen:
//     - the loop only reads it                          (`n`, and constants)
//     - or it is a whole-number counter: its value is a known whole number
//       when the loop is entered, and the loop only sets it to a constant or
//       steps it by a constant of at most MAX_COUNTER_STEP          (`i++`)
//   Accumulators (`total += x`) stay in their frame slot.
//   If VM registers ever become `double`, this restriction can simply go:
//   register and slot would then round identically.
//

#include "passes.h"
#include "../analysis/control_flow.h"
#include "../analysis/loops.h"
#include "../analysis/liveness.h"
#include "../analysis/known_values.h"

#define MAX_FUNCTION_INSTRUCTIONS 6000
/* A counter stepped by at most this much needs 2^43 trips to leave the range in
   which a double is exact - far more than any loop runs. */
#define MAX_COUNTER_STEP 1024

namespace {

/** Is `instr` an access to a local's number that translate() can rewrite? Returns the locals in a / b. */
bool local_operands(const ir_instruction &instr, int &first, int &second, bool &writesFirst, bool &writesSecond) {
    first = second = -1;
    writesFirst = writesSecond = false;

    switch(instr.op) {
        case Opcode::LOADL:    first = instr.b; return true;                        // regs[a] = local
        case Opcode::SMOVR_2:  first = instr.b; writesFirst = true; return true;    // local = regs[a]
        case Opcode::ISTOREL:  first = instr.a; writesFirst = true; return true;    // local = constant
        case Opcode::IADDL: case Opcode::ISUBL: case Opcode::IMULL: case Opcode::IDIVL:
            first = instr.a; writesFirst = true; return true;                       // local op= constant
        case Opcode::ADDL: case Opcode::SUBL: case Opcode::MULL: case Opcode::DIVL:
            first = instr.b; writesFirst = true; return true;                       // local op= regs[a]
        case Opcode::SMOVR_4:                                                       // local a = local b
            first = instr.a; writesFirst = true; second = instr.b; return true;
        case Opcode::IPUSHL:   first = instr.a; return true;                        // push local
        case Opcode::IPOPL:    first = instr.a; writesFirst = true; return true;    // local = pop
        default: return false;
    }
}

void become(ir_instruction &instr, uint8_t op, int32_t a = 0, int32_t b = 0, int32_t c = 0) {
    instr.op = op;
    instr.a = a; instr.b = b; instr.c = c;
    instr.modified = true;
}

/** Rewrite one access. registerOf[n] is the register of local n, or -1. */
void translate(ir_instruction &instr, const std::vector<int> &registerOf) {
    auto reg = [&](int32_t local) { return local >= 0 && local < (int32_t)registerOf.size() ? registerOf[local] : -1; };

    switch(instr.op) {
        case Opcode::LOADL:   if(reg(instr.b) >= 0) become(instr, Opcode::MOVR, instr.a, reg(instr.b)); break;
        case Opcode::SMOVR_2: if(reg(instr.b) >= 0) become(instr, Opcode::MOVR, reg(instr.b), instr.a); break;
        case Opcode::ISTOREL: if(reg(instr.a) >= 0) become(instr, Opcode::MOVI, reg(instr.a), instr.b); break;
        case Opcode::IADDL:   if(reg(instr.a) >= 0) become(instr, Opcode::IADD, reg(instr.a), instr.b); break;
        case Opcode::ISUBL:   if(reg(instr.a) >= 0) become(instr, Opcode::ISUB, reg(instr.a), instr.b); break;
        case Opcode::IMULL:   if(reg(instr.a) >= 0) become(instr, Opcode::IMUL, reg(instr.a), instr.b); break;
        case Opcode::IDIVL:   if(reg(instr.a) >= 0) become(instr, Opcode::IDIV, reg(instr.a), instr.b); break;
        // local op= regs[a]   ->   R = R op regs[a]
        case Opcode::ADDL:    if(reg(instr.b) >= 0) become(instr, Opcode::ADD, reg(instr.b), instr.a, reg(instr.b)); break;
        case Opcode::SUBL:    if(reg(instr.b) >= 0) become(instr, Opcode::SUB, reg(instr.b), instr.a, reg(instr.b)); break;
        case Opcode::MULL:    if(reg(instr.b) >= 0) become(instr, Opcode::MUL, reg(instr.b), instr.a, reg(instr.b)); break;
        case Opcode::DIVL:    if(reg(instr.b) >= 0) become(instr, Opcode::DIV, reg(instr.b), instr.a, reg(instr.b)); break;
        case Opcode::SMOVR_4: {
            int to = reg(instr.a), from = reg(instr.b);
            if(to >= 0 && from >= 0) become(instr, Opcode::MOVR, to, from);
            else if(to >= 0)         become(instr, Opcode::LOADL, to, instr.b);     // R = other local
            else if(from >= 0)       become(instr, Opcode::SMOVR_2, from, instr.a); // other local = R
            break;
        }
        case Opcode::IPUSHL:  if(reg(instr.a) >= 0) become(instr, Opcode::RSTORE, reg(instr.a)); break;
        case Opcode::IPOPL:   if(reg(instr.a) >= 0) become(instr, Opcode::LOADVAL, reg(instr.a)); break;
        default: break;
    }
}

/** Promote the locals of one loop. Returns true if the function was changed (indexes are stale afterwards). */
bool promote_one(function_ir &ir, std::vector<int> &doneHeaders) {
    control_flow cfg;
    loop_forest forest;
    build_control_flow(ir, cfg);
    find_loops(ir, cfg, forest);
    if(!forest.valid) return false;

    liveness live;
    std::vector<machine_state> known;
    bool haveLiveness = false;
    int slots = ir.frameSlots > 0 ? ir.frameSlots : 1;

    for(loop_info &loop : forest.loops) {
        basic_block &header = cfg.blocks[loop.header];
        int headerId = ir.code[header.first].id;

        // each loop gets one go (the ids of its first instruction never change)
        bool done = false;
        for(int id : doneHeaders) done = done || id == headerId;
        if(done) continue;
        doneHeaders.push_back(headerId);

        // ---- entered at one place, by running into the header
        if(loop.entries.size() != 1 || header.first == 0) continue;
        basic_block &entry = cfg.blocks[loop.entries[0]];
        if(entry.last + 1 != header.first) continue;
        {
            const ir_instruction &last = ir.code[entry.last];
            if(instruction_info_of(last.op).flags & f_terminator) continue;       // it jumps here instead
            if(last.target != IR_NONE && resolve_reference(ir, last.target) == headerId) continue;
        }

        // ---- left at one place: a conditional jump back into the loop, falling out behind it
        int exitEdges = 0, bottom = -1;
        for(int b : loop.blocks) {
            for(int next : cfg.blocks[b].successors) {
                if(!loop.contains[next]) { exitEdges++; bottom = b; }
            }
        }
        if(exitEdges != 1) continue;

        int bottomIndex = cfg.blocks[bottom].last;
        {
            const ir_instruction &test = ir.code[bottomIndex];
            if(test.op != Opcode::JE && test.op != Opcode::JNE) continue;
            if(bottomIndex + 1 >= (int)ir.code.size()) continue;

            int targetId = resolve_reference(ir, test.target);
            int targetIndex = targetId >= 0 ? index_of(ir, targetId) : -1;
            if(targetIndex < 0 || !loop.contains[cfg.blockOf[targetIndex]]) continue;  // the jump is the way out
            if(loop.contains[cfg.blockOf[bottomIndex + 1]]) continue;
        }

        // ---- what the loop does: registers it uses, locals it touches and how
        register_set usedRegisters = 0;
        std::vector<int> uses(slots, 0);
        std::vector<char> written(slots, 0), blocked(slots, 0);
        std::vector<char> rounded(slots, 0);    // written with a value that may need the slot's rounding
        bool suitable = true;

        for(int b : loop.blocks) {
            for(int i = cfg.blocks[b].first; i <= cfg.blocks[b].last && suitable; i++) {
                const ir_instruction &instr = ir.code[i];
                const instruction_info &info = instruction_info_of(instr.op);

                if((info.flags & (f_call | f_opaque | f_indirect_jump | f_reads_pc)) || instr.pcConstant) {
                    suitable = false;
                    break;
                }

                // addressing relative to sp or the stack base could name a frame slot behind our back
                if(info.a == k_sp_rel || info.b == k_sp_rel || info.c == k_sp_rel
                   || info.a == k_abs_slot || info.b == k_abs_slot || info.c == k_abs_slot) {
                    suitable = false;
                    break;
                }

                register_set reads, writes;
                register_effects(instr, reads, writes);
                usedRegisters |= reads | writes;

                int first, second; bool writesFirst, writesSecond;
                if(local_operands(instr, first, second, writesFirst, writesSecond)) {
                    for(int n : { first, second }) {
                        if(n < 0) continue;
                        if(n >= slots) { suitable = false; break; }
                        uses[n]++;
                    }
                    if(suitable && writesFirst) {
                        written[first] = 1;

                        // a "counter" write: set to a whole number, or stepped by a small one
                        bool counterWrite = instr.op == Opcode::ISTOREL
                                || ((instr.op == Opcode::IADDL || instr.op == Opcode::ISUBL)
                                    && instr.b >= -MAX_COUNTER_STEP && instr.b <= MAX_COUNTER_STEP);
                        if(!counterWrite) rounded[first] = 1;
                    }
                } else {
                    // any other way of touching a local's number: hands off that local
                    const operand_kind kinds[3] = { info.a, info.b, info.c };
                    const int32_t values[3] = { instr.a, instr.b, instr.c };
                    for(int k = 0; k < 3; k++) {
                        if(kinds[k] != k_var_r && kinds[k] != k_var_w && kinds[k] != k_var_rw) continue;
                        if(values[k] < 0 || values[k] >= slots) { suitable = false; break; }
                        blocked[values[k]] = 1;
                    }
                    // returnval writes slot 0 without naming it
                    if(instr.op == Opcode::RETURNVAL) blocked[0] = 1;
                }
            }
        }
        if(!suitable) continue;

        if(!haveLiveness) {
            compute_liveness(ir, cfg, live);
            if(!analyze_known_values(ir, cfg, known)) return false;
            haveLiveness = true;
        }

        // ---- locals the loop writes: only whole-number counters (see PRECISION in the header)
        {
            machine_state atEntry = known[loop.entries[0]];
            for(int i = entry.first; i <= entry.last; i++)
                apply_instruction(ir.code[i], atEntry);

            for(int n = 0; n < slots; n++) {
                if(written[n] && (rounded[n] || !is_known(atEntry.locals[n]))) blocked[n] = 1;
            }
        }

        // ---- free registers: untouched by the loop, and not carrying a value past it
        register_set neededAfter = live.registersAfter[bottomIndex] | live.registersAfter[entry.last];

        static const int order[] = { ECF, EDF, EHF, ECX, CX, EGX, EBX, ADX };
        std::vector<int> freeRegisters;
        for(int r : order) {
            if(!((usedRegisters | neededAfter) & REG_BIT(r))) freeRegisters.push_back(r);
        }
        if(freeRegisters.empty()) continue;

        // ---- the most used locals get the registers
        std::vector<int> registerOf(slots, -1);
        int promoted = 0;
        while(promoted < (int)freeRegisters.size()) {
            int best = -1;
            for(int n = 0; n < slots; n++) {
                if(blocked[n] || registerOf[n] >= 0 || uses[n] == 0) continue;
                if(best < 0 || uses[n] > uses[best]) best = n;
            }
            if(best < 0) break;
            registerOf[best] = freeRegisters[promoted++];
        }

        // ---- registers left over go to the constants the loop keeps reloading
        // (`i < 1000` loads 1000 into a register on every trip)
        std::vector<std::pair<int32_t, int>> constants;     // value, register
        while(promoted + (int)constants.size() < (int)freeRegisters.size()) {
            bool found = false;
            int32_t best = 0; int bestCount = 0;

            for(int b : loop.blocks) {
                for(int i = cfg.blocks[b].first; i <= cfg.blocks[b].last; i++) {
                    const ir_instruction &instr = ir.code[i];
                    if(instr.op != Opcode::MOVI) continue;

                    bool taken = false;
                    for(auto &entry : constants) taken = taken || entry.first == instr.b;
                    if(taken) continue;

                    int count = 0;
                    for(int b2 : loop.blocks) {
                        for(int j = cfg.blocks[b2].first; j <= cfg.blocks[b2].last; j++)
                            if(ir.code[j].op == Opcode::MOVI && ir.code[j].b == instr.b) count++;
                    }
                    if(!found || count > bestCount) { found = true; best = instr.b; bestCount = count; }
                }
            }

            if(!found) break;
            constants.push_back(std::make_pair(best, freeRegisters[promoted + (int)constants.size()]));
        }

        if(promoted == 0 && constants.empty()) continue;

        // ---- 3. the loop body (in place: no instruction is added or removed)
        for(int b : loop.blocks) {
            for(int i = cfg.blocks[b].first; i <= cfg.blocks[b].last; i++) {
                ir_instruction &instr = ir.code[i];
                translate(instr, registerOf);

                // regs[a] = constant   ->   regs[a] = the register holding that constant
                if(instr.op == Opcode::MOVI) {
                    for(auto &entry : constants) {
                        if(entry.first == instr.b && entry.second != instr.a) {
                            become(instr, Opcode::MOVR, instr.a, entry.second);
                            break;
                        }
                    }
                }
            }
        }

        // ---- 4. behind the loop: store what changed. (Inserted first, it is the later position.)
        // A jump to the instruction after the loop still lands behind these stores,
        // which is right: such a jump did not come out of the loop.
        std::vector<ir_instruction> stores, loads;
        for(int n = 0; n < slots; n++) {
            if(registerOf[n] < 0) continue;
            loads.push_back(make_instruction(ir, Opcode::LOADL, registerOf[n], n));
            if(written[n]) stores.push_back(make_instruction(ir, Opcode::SMOVR_2, registerOf[n], n));
        }
        for(auto &entry : constants)
            loads.push_back(make_instruction(ir, Opcode::MOVI, entry.second, entry.first));
        ir.code.insert(ir.code.begin() + bottomIndex + 1, stores.begin(), stores.end());

        // ---- 2. in front of the loop. The jump back to the top targets the header's
        // first instruction, which is now behind these loads: they run once.
        ir.code.insert(ir.code.begin() + header.first, loads.begin(), loads.end());

        ir.changed = true;
        return true;
    }

    return false;
}

} // namespace

bool pass_loop_registers(function_ir &ir) {
    if(!ir.failure.empty() || !ir.tries.empty()) return false;

    bool changed = false;
    std::vector<int> doneHeaders;
    // one loop per call, innermost first: the blocks move every time
    for(int round = 0; round < 64 && ir.code.size() < MAX_FUNCTION_INSTRUCTIONS && promote_one(ir, doneHeaders); round++)
        changed = true;

    return changed;
}
