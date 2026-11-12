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
// HOW
//   analysis/known_values.h works out what is known where (a standard forward
//   data flow analysis); this pass rewrites the instructions accordingly.
//

#include "passes.h"
#include "../analysis/liveness.h"
#include "../analysis/known_values.h"
#include <cstdint>

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

    std::vector<machine_state> in;
    if(!analyze_known_values(ir, cfg, in)) return false;
    size_t blocks = cfg.blocks.size();

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
