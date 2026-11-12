//
// Pass: loop unrolling   (-O3, run before loop rotation)
// =====================================================
// A loop that is certain to run a small, known number of times is replaced by
// that many copies of its body, with no counter test and no jumps.
//
//     for(k = 0; k < 4; k++) { total += k * i; }
//
//     before                                after unrolling
//     ----------------------------------    -----------------------------------
//             istorel 0, k                          istorel 0, k
//       top:  loadl cx, k                           loadl ecx, k   ; body, k = 0
//             movi #4, ecx                          ...
//             lt cx, ecx                            iaddl 1, k
//             jne exit                              loadl ecx, k   ; body, k = 1
//             loadl ecx, k   ; body                 ...
//             ...                                   iaddl 1, k
//             iaddl 1, k                            ...            ; k = 2, k = 3
//             jmp top                               jmp exit
//       exit:                                 (the old loop is now unreachable
//                                              and is deleted)
//
//   Constant propagation then knows k in every copy (0, 1, 2, 3), so `k * i`
//   becomes `0`, `i`, `i * 2`, `i * 3` and the counter itself disappears.
//
// HOW: THE LOOP IS RUN AT COMPILE TIME
//   Nothing about the shape of the loop is pattern-matched. Starting from what
//   is known where the loop is entered (analysis/known_values.h), the loop is
//   executed symbolically, one instruction after the other:
//     - every conditional jump on the way must have a KNOWN condition, so there
//       is never a doubt about which way it goes
//     - values that are not known (`i` above) are simply carried as unknown;
//       that is fine as long as no jump depends on them
//     - the run ends when control leaves the loop
//   The list of instructions executed on that run is exactly what the program
//   would execute, in order. Copied out as straight-line code, without the
//   jumps (their outcome is already decided), it replaces the loop.
//
//   The loop is given up on - left as it is - as soon as a jump depends on
//   something unknown (the usual case: `i < n`), or the run gets longer than
//   MAX_UNROLLED_INSTRUCTIONS / MAX_TRIPS, or it leaves through `ret`/`throw`.
//
// WHICH LOOPS
//   Entered at one place by running into it (like the loop register pass), no
//   try/catch/finally in the function.
//
// COST
//   Code size: the body times the trip count, bounded by the limits below.
//

#include "passes.h"
#include "../analysis/control_flow.h"
#include "../analysis/loops.h"
#include "../analysis/known_values.h"

/* the straight-line replacement may not be longer than this */
#define MAX_UNROLLED_INSTRUCTIONS 96
/* ...nor may the loop go round more often than this */
#define MAX_TRIPS 16
#define MAX_FUNCTION_INSTRUCTIONS 6000

namespace {

/** Unroll one loop. Returns true if the function was changed (indexes are stale afterwards). */
bool unroll_one(function_ir &ir, std::vector<int> &triedHeaders) {
    control_flow cfg;
    loop_forest forest;
    build_control_flow(ir, cfg);
    find_loops(ir, cfg, forest);
    if(!forest.valid) return false;

    std::vector<machine_state> in;
    bool analyzed = false;

    for(loop_info &loop : forest.loops) {
        basic_block &header = cfg.blocks[loop.header];
        int headerId = ir.code[header.first].id;

        bool tried = false;
        for(int id : triedHeaders) tried = tried || id == headerId;
        if(tried) continue;
        triedHeaders.push_back(headerId);

        // ---- entered at one place, by running into the header
        if(loop.entries.size() != 1 || header.first == 0) continue;
        basic_block &entry = cfg.blocks[loop.entries[0]];
        if(entry.last + 1 != header.first) continue;
        {
            const ir_instruction &last = ir.code[entry.last];
            if(instruction_info_of(last.op).flags & f_terminator) continue;
            if(last.target != IR_NONE && resolve_reference(ir, last.target) == headerId) continue;
        }

        if(!analyzed) {
            if(!analyze_known_values(ir, cfg, in)) return false;
            analyzed = true;
        }

        // ---- what is known when the loop is entered: the end of the entry block
        machine_state state = in[loop.entries[0]];
        for(int i = entry.first; i <= entry.last; i++)
            apply_instruction(ir.code[i], state);

        // ---- run the loop
        std::vector<int> trace;     // indexes of the instructions executed, jumps left out
        int trips = 0, exitIndex = -1;
        int pc = header.first;
        bool ok = true;

        while(ok) {
            if(pc < 0 || pc >= (int)ir.code.size()) { ok = false; break; }
            if(!loop.contains[cfg.blockOf[pc]]) { exitIndex = pc; break; }       // left the loop
            if(pc == header.first && ++trips > MAX_TRIPS) { ok = false; break; }

            const ir_instruction &instr = ir.code[pc];
            const instruction_info &info = instruction_info_of(instr.op);

            if(instr.pcConstant || (info.flags & (f_indirect_jump | f_reads_pc))) { ok = false; break; }

            if(instr.op == Opcode::JMP || instr.op == Opcode::JE || instr.op == Opcode::JNE) {
                bool taken = true;
                if(instr.op != Opcode::JMP) {
                    const known_value &condition = state.registers[CMT];
                    if(!is_known(condition)) { ok = false; break; }              // depends on a run time value
                    taken = (condition.value != 0) == (instr.op == Opcode::JE);
                }

                if(taken) {
                    int target = resolve_reference(ir, instr.target);
                    pc = target >= 0 ? index_of(ir, target) : -1;
                } else {
                    pc++;
                }
                continue;   // the jump itself is not part of the straight-line code
            }

            // any other way of changing direction (skip, ret, throw, ...): not handled
            if(info.flags & (f_jump | f_cond_jump | f_terminator)) { ok = false; break; }

            trace.push_back(pc);
            if((int)trace.size() > MAX_UNROLLED_INSTRUCTIONS) { ok = false; break; }

            apply_instruction(instr, state);
            pc++;
        }

        if(!ok || exitIndex < 0 || trips < 1) continue;

        // ---- the straight-line code, in front of the old loop
        std::vector<ir_instruction> unrolled;
        for(int index : trace) {
            const ir_instruction &original = ir.code[index];
            unrolled.push_back(make_instruction(ir, original.op, original.a, original.b, original.c));
        }
        ir_instruction leave = make_instruction(ir, Opcode::JMP, 0);
        leave.target = ir.code[exitIndex].id;
        unrolled.push_back(leave);

        // The entry block runs into the new code, which jumps past the old loop.
        // Nothing else leads into the old loop, so it is unreachable now.
        ir.code.insert(ir.code.begin() + header.first, unrolled.begin(), unrolled.end());

        ir.changed = true;
        return true;
    }

    return false;
}

} // namespace

bool pass_loop_unrolling(function_ir &ir) {
    if(!ir.failure.empty() || !ir.tries.empty()) return false;

    bool changed = false;
    std::vector<int> triedHeaders;
    // one loop per call, innermost first: the blocks move every time
    for(int round = 0; round < 32 && ir.code.size() < MAX_FUNCTION_INSTRUCTIONS && unroll_one(ir, triedHeaders); round++) {
        changed = true;
        pass_unreachable_code(ir);  // drop the old loop now: the loop analysis wants every block reachable
    }

    return changed;
}
