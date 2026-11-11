//
// Pass: dead code   (-O2)
// =======================
// Removes everything that does not contribute to something the program can
// observe: unused computations, variables that are only ever used to update
// themselves, `if`s that guard nothing, and whole loops whose results are never
// used. This is the "aggressive dead code elimination" of optimizing compilers.
//
//     source                                   what is left
//     -------------------------------------    ------------------------------
//     def f(n: var) {                          def f(n: var) {
//         total : var = 0;                         println("done");
//         for(i : var = 0; i < n; i++) {       }
//             total += i * 2;
//         }
//         println("done");
//     }
//
//   `total` is never read except to compute its own next value, so every
//   instruction that writes it is dead. That leaves a loop that only counts,
//   and a loop that guards nothing is dead as well.
//
// HOW IT DECIDES
//   1. ESSENTIAL instructions stay no matter what: anything with an effect
//      outside the function's own registers and number locals, anything that
//      can raise an exception, anything that touches the operand stack, calls,
//      returns. (instruction_set.h has the facts, is_plain_computation() the rule.)
//   2. A plain computation stays only if something that stays needs its result
//      ("strong" liveness, analysis/liveness.h).
//   3. A conditional branch stays only if something that stays is CONTROL
//      DEPENDENT on it, i.e. the branch decides whether that thing runs
//      (analysis/post_dominators.h).
//   2 and 3 feed each other (a kept branch needs its condition, a kept
//   instruction needs the branches around it), so they are repeated until
//   nothing new has to be kept.
//
//   Then: unneeded computations are deleted, and an unneeded conditional branch
//   is replaced by a jump to the place where its two ways meet again (its
//   immediate post-dominator). For a loop that place is the loop exit, so the
//   body becomes unreachable and the unreachable code pass removes it.
//
// WHAT THIS ASSUMES - please read
//   A loop with no observable effect is assumed to END. If it is removed, a
//   program that used an empty counting loop as a delay loses the delay, and a
//   loop that would have spun forever on local variables alone simply ends.
//   (C and C++ compilers make the same assumption.) Loops that are safe from
//   this by construction:
//     - `for(;;) { }` / `while(true) { }`: there is no way out of the loop, so
//       there is nothing to jump to; such functions are left alone
//     - a loop waiting for a field or a global to change (`while(!done) { }`):
//       reading a field can raise a null exception, which makes it essential
//     - any loop that calls a function, sleeps, locks, or does io
//
// LIMITS
//   Functions with try/catch/finally are skipped by this pass (the simpler
//   dead stores pass still handles them).
//

#include "passes.h"
#include "../analysis/liveness.h"
#include "../analysis/post_dominators.h"

static bool is_conditional_jump(const ir_instruction &instr) {
    const instruction_info &info = instruction_info_of(instr.op);
    return (info.flags & f_cond_jump) != 0;
}

/** One round. Returns true if it changed the function. */
static bool eliminate_once(function_ir &ir) {
    control_flow cfg;
    post_dominators pdom;
    build_control_flow(ir, cfg);
    compute_post_dominators(ir, cfg, pdom);
    if(!pdom.valid) return false;

    size_t count = ir.code.size();
    size_t blocks = cfg.blocks.size();

    // ---- 1. what stays no matter what
    std::vector<char> essential(count, 0), plain(count, 0);
    for(size_t i = 0; i < count; i++) {
        plain[i] = is_plain_computation(ir.code[i]);
        essential[i] = !plain[i] && !is_conditional_jump(ir.code[i]);
    }

    // ---- 2 + 3. grow the set of kept instructions until it is stable
    std::vector<char> branchNeeded(blocks, 0);
    std::vector<char> kept(count, 0);
    liveness live;

    for(;;) {
        // a conditional branch we decided to keep is essential: its condition is needed
        std::vector<char> staying = essential;
        for(size_t b = 0; b < blocks; b++) {
            if(branchNeeded[b]) staying[cfg.blocks[b].last] = 1;
        }

        compute_liveness(ir, cfg, live, &staying);
        if(!live.localsKnown) return false; // cannot reason about this function's locals

        for(size_t i = 0; i < count; i++) {
            ir_instruction &instr = ir.code[i];

            if(staying[i]) { kept[i] = 1; continue; }
            if(!plain[i])  { kept[i] = 0; continue; } // a conditional branch not (yet) needed

            register_set reads, writes;
            register_effects(instr, reads, writes);
            bool needed = (writes & live.registersAfter[i]) != 0;

            const instruction_info &info = instruction_info_of(instr.op);
            const operand_kind kinds[3] = { info.a, info.b, info.c };
            const int32_t values[3] = { instr.a, instr.b, instr.c };
            for(int k = 0; k < 3 && !needed; k++) {
                if(kinds[k] == k_var_w || kinds[k] == k_var_rw)
                    needed = live.local_live_after(i, values[k]);
            }

            kept[i] = needed;
        }

        // every branch that decides whether a kept instruction runs is needed
        bool grew = false;
        for(size_t b = 0; b < blocks; b++) {
            bool hasKeptContent = false;
            for(int i = cfg.blocks[b].first; i <= cfg.blocks[b].last && !hasKeptContent; i++) {
                // a plain `jmp` is plumbing, not content
                hasKeptContent = kept[i] && ir.code[i].op != Opcode::JMP;
            }
            if(!hasKeptContent) continue;

            for(int decider : pdom.dependsOn[b]) {
                if(!branchNeeded[decider]) {
                    branchNeeded[decider] = 1;
                    grew = true;
                }
            }
        }

        if(!grew) break;
    }

    // ---- rewrite. First the branches (they need the block layout as it is now)...
    bool changed = false;
    std::vector<int> firstIdOfBlock(blocks);
    for(size_t b = 0; b < blocks; b++) firstIdOfBlock[b] = ir.code[cfg.blocks[b].first].id;

    for(size_t b = 0; b < blocks; b++) {
        ir_instruction &last = ir.code[cfg.blocks[b].last];
        if(!is_conditional_jump(last) || branchNeeded[b]) continue;

        int meetingPoint = pdom.immediate[b];
        if(meetingPoint == PDOM_EXIT) continue; // both ways leave the function separately: keep it

        // nothing that stays depends on which way this goes: go straight to where the ways meet
        last.op = Opcode::JMP;
        last.a = last.b = last.c = 0;
        last.target = firstIdOfBlock[meetingPoint];
        last.pcConstant = false;
        last.modified = true;
        changed = true;
    }

    // ...then the computations nobody needs, back to front so indexes stay valid
    for(size_t i = count; i-- > 0; ) {
        if(plain[i] && !kept[i]) {
            remove_instruction(ir, i);
            changed = true;
        }
    }

    return changed;
}

bool pass_dead_code(function_ir &ir) {
    bool changed = false;

    // each round can expose more (a removed loop makes the values it read dead)
    for(int round = 0; round < 8 && eliminate_once(ir); round++)
        changed = true;

    return changed;
}
