//
// Pass: dead stores (unused values and unused variables)
// ======================================================
// Removes an instruction when ALL of the following hold:
//
//   - the only thing it does is write registers and/or the number part of a
//     local variable (no stack traffic, no heap write, no call, no jump)
//   - it cannot raise an exception
//   - nothing reads what it wrote before that is overwritten again, or before
//     the function ends (liveness.h)
//
// This is what removes a local variable that is assigned but never used, and
// the computation that fed it, one instruction at a time: once the store is
// gone, the instruction that produced the stored value has no reader either.
//
//     source:   x : var = a + 5;      // x is never read
//
//     before                         after
//     -------------------------      -------------------------
//       loadl  cx, fp+1                (all four removed)
//       movi   5, ecx
//       add    cx, ecx -> bmr
//       smovr_2 bmr, fp+2
//

#include "passes.h"
#include "../analysis/liveness.h"

/** flags an instruction may have and still be "just a computation" */
#define HARMLESS_FLAGS (f_reads_ptr | f_reads_heap | f_state_check)

bool pass_dead_stores(function_ir &ir) {
    bool changedAtAll = false;
    bool changed = true;

    while(changed) {
        changed = false;

        control_flow cfg;
        liveness live;
        build_control_flow(ir, cfg);
        compute_liveness(ir, cfg, live);

        // back to front: removing an instruction does not disturb the indexes
        // (and liveness results) of the ones before it
        for(size_t i = ir.code.size(); i-- > 0; ) {
            ir_instruction &instr = ir.code[i];
            const instruction_info &info = instruction_info_of(instr.op);

            if(info.flags & ~HARMLESS_FLAGS) continue;
            if(info.popsNumbers || info.popsObjects || info.pushesNumbers || info.pushesObjects) continue;

            // operands the analysis does not track mean "it writes somewhere we cannot see"
            const operand_kind kinds[3] = { info.a, info.b, info.c };
            const int32_t values[3] = { instr.a, instr.b, instr.c };
            bool tracked = true, writesSomething = false, allDead = true;

            for(int k = 0; k < 3; k++) {
                switch(kinds[k]) {
                    case k_sp_rel: case k_abs_slot: case k_obj_w: case k_slot_addr:
                        tracked = false;
                        break;
                    case k_var_w: case k_var_rw:
                        writesSomething = true;
                        if(!live.localsKnown || live.local_live_after(i, values[k])) allDead = false;
                        break;
                    default:
                        break;
                }
            }
            if(!tracked) continue;

            register_set reads, writes;
            register_effects(instr, reads, writes);
            if(writes != 0) {
                writesSomething = true;
                if(writes & live.registersAfter[i]) allDead = false;
            }

            if((writesSomething && allDead) || instr.op == Opcode::NOP) {
                remove_instruction(ir, i);
                changed = true;
            }
        }

        changedAtAll |= changed;
    }

    return changedAtAll;
}
