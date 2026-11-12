//
// Pass: copy propagation   (-O1)
// ==============================
// Removes register-to-register moves by letting the instructions around them
// use the right register in the first place. Works inside one basic block.
//
// 1. FORWARD: after `movr d, s` the two registers hold the same value, so an
//    instruction that reads d can read s instead. When nothing reads d any
//    more, the dead stores pass deletes the move.
//
//        movr cx, ecf            movr cx, ecf       <- now unused, removed later
//        movr ecx, ehf     ->    movr ecx, ehf      <- same
//        lt cx, ecx              lt ecf, ehf
//
// 2. BACKWARD: `X -> r` directly followed by `movr d, r`, with r not needed
//    afterwards: X can write d directly.
//
//        add cx, ecx -> bmr      add cx, ecx -> edf
//        movr edf, bmr     ->
//
// Both are everyday leftovers of the code generator (it computes into bmr and
// then moves the result where it is wanted) and exactly what the loop register
// pass produces on purpose (see pass_loop_registers.cpp).
//
// WHAT STOPS A FORWARD SUBSTITUTION
//   - d or s is written again: they no longer hold the same value
//   - a call or an interrupt: the callee may overwrite any register
//   - the end of the block: another path may arrive with different contents
//   Reads that are part of the instruction itself rather than an operand (`je`
//   reads cmt, `brh` reads adx) cannot be redirected; the move simply stays.
//

#include "passes.h"
#include "../analysis/control_flow.h"
#include "../analysis/liveness.h"

/** 1. forward. Returns true if any operand was changed. */
static bool forward_copies(function_ir &ir, control_flow &cfg) {
    bool changed = false;

    for(basic_block &block : cfg.blocks) {
        for(int m = block.first; m < block.last; m++) {
            const ir_instruction &move = ir.code[m];
            if(move.op != Opcode::MOVR || move.a == move.b) continue;
            int d = move.a, s = move.b;

            for(int i = m + 1; i <= block.last; i++) {
                ir_instruction &instr = ir.code[i];
                const instruction_info &info = instruction_info_of(instr.op);
                if(info.flags & (f_call | f_opaque)) break;

                // reads happen before the instruction's own writes
                const operand_kind kinds[3] = { info.a, info.b, info.c };
                int32_t *values[3] = { &instr.a, &instr.b, &instr.c };
                for(int k = 0; k < 3; k++) {
                    if(kinds[k] == k_reg_r && *values[k] == d) {
                        *values[k] = s;
                        instr.modified = true;
                        changed = true;
                    }
                }

                register_set reads, writes;
                register_effects(instr, reads, writes);
                if(writes & (REG_BIT(d) | REG_BIT(s))) break;
            }
        }
    }

    return changed;
}

/** 2. backward. Returns true if any move was removed. */
static bool coalesce_results(function_ir &ir, control_flow &cfg) {
    liveness live;
    compute_liveness(ir, cfg, live);

    std::vector<int> remove;

    for(basic_block &block : cfg.blocks) {
        for(int m = block.first + 1; m <= block.last; m++) {
            const ir_instruction &move = ir.code[m];
            if(move.op != Opcode::MOVR || move.a == move.b) continue;
            int d = move.a, r = move.b;

            if(live.registersAfter[m] & REG_BIT(r)) continue;       // r is still wanted
            if(!remove.empty() && remove.back() == m - 1) continue; // the producer is itself a move being removed

            ir_instruction &producer = ir.code[m - 1];
            const instruction_info &info = instruction_info_of(producer.op);
            if(info.flags & (f_call | f_opaque)) continue;
            if(info.implicitWrites & (REG_BIT(r) | REG_BIT(d))) continue;

            // exactly one operand writes r, and it is a pure write
            const operand_kind kinds[3] = { info.a, info.b, info.c };
            int32_t *values[3] = { &producer.a, &producer.b, &producer.c };
            int target = -1, writers = 0;
            for(int k = 0; k < 3; k++) {
                if(!writes_register(kinds[k]) || *values[k] != r) continue;
                writers++;
                if(kinds[k] == k_reg_w) target = k;
            }
            if(writers != 1 || target < 0) continue;

            *values[target] = d;
            producer.modified = true;
            remove.push_back(m);
        }
    }

    for(size_t i = remove.size(); i-- > 0; )
        remove_instruction(ir, (size_t)remove[i]);

    return !remove.empty();
}

bool pass_copy_propagation(function_ir &ir) {
    bool changed = false;

    // `movr r, r` does nothing
    for(size_t i = ir.code.size(); i-- > 0; ) {
        if(ir.code[i].op == Opcode::MOVR && ir.code[i].a == ir.code[i].b) {
            remove_instruction(ir, i);
            changed = true;
        }
    }

    control_flow cfg;
    build_control_flow(ir, cfg);
    if(forward_copies(ir, cfg)) changed = true;
    if(coalesce_results(ir, cfg)) changed = true;   // (only removes instructions: block ranges stay usable until here)

    return changed;
}
