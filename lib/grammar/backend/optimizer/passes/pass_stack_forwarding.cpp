//
// Pass: stack forwarding
// ======================
// The code generator moves most numbers through the operand stack: it pushes a
// value and pops it again a moment later into the place that needs it. This
// pass finds such a push/pop pair and replaces it with one direct move, or with
// nothing at all when the popped value is never used.
//
//   push (source)                      pop (destination)
//   --------------------------------   ------------------------------
//   istore <constant>                  loadval <register>
//   rstore <register>                  ipopl   <local>
//   ipushl <local>
//
// 1. THE POPPED VALUE IS NEVER USED: both instructions are removed.
//    (`i++` as a statement compiles to "push old i, add 1, pop old i": the old
//     value is thrown away)
//
//        ipushl 1                 ->      iaddl 1, fp+1
//        iaddl  1, fp+1
//        loadval ebx        ; ebx is not read afterwards
//
// 2. OTHERWISE the pair becomes a direct move at the place of the pop:
//
//        istore 3   ... loadval ecx      ->   movi 3, ecx
//        ipushl 1   ... loadval ecx      ->   loadl ecx, fp+1
//        rstore bmr ... ipopl 2          ->   smovr_2 bmr, fp+2
//        istore 0   ... ipopl 1          ->   istorel fp+1, 0
//        ipushl 1   ... ipopl 2          ->   smovr_4 fp+2, fp+1
//        rstore cx  ... loadval ecx      ->   movr ecx, cx        (see NOTE)
//
// WHEN IS A PAIR A PAIR?
//   The push and the pop must be in the same basic block, and nothing between
//   them may touch the operand stack, address it relative to sp, or call
//   anything. For case 2 the source must also still hold the pushed value when
//   the pop is reached (nothing in between writes that register / local).
//
// NOTE ON PRECISION (register -> register)
//   A stack slot holds a `double`, a register a `long double`. Pushing and
//   popping a register therefore rounds it to double. A direct `movr` would
//   not, and a program could print a different last digit. So register ->
//   register forwarding is only done when the register's value is known to be
//   exactly a double already (it was loaded from a local, the stack or a
//   constant, or is a 0/1 comparison result).
//

#include "passes.h"
#include "../analysis/liveness.h"

enum value_place { place_none, place_constant, place_register, place_local };

struct place {
    value_place kind;
    int32_t value;   // the constant, register number or local slot
};

static place pushed_from(const ir_instruction &instr) {
    switch(instr.op) {
        case Opcode::ISTORE: return { place_constant, instr.a };
        case Opcode::RSTORE: return { place_register, instr.a };
        case Opcode::IPUSHL: return { place_local, instr.a };
        default:             return { place_none, 0 };
    }
}

static place popped_into(const ir_instruction &instr) {
    switch(instr.op) {
        case Opcode::LOADVAL: return { place_register, instr.a };
        case Opcode::IPOPL:   return { place_local, instr.a };
        default:              return { place_none, 0 };
    }
}

/** May sit between a push and its pop without disturbing the stack? */
static bool leaves_stack_alone(const ir_instruction &instr) {
    const instruction_info &info = instruction_info_of(instr.op);

    if(info.popsNumbers || info.popsObjects || info.pushesNumbers || info.pushesObjects) return false;
    if(info.flags & (f_call | f_opaque | f_jump | f_cond_jump | f_indirect_jump | f_terminator)) return false;
    if(instr.op == Opcode::SWAP) return false;
    if(info.a == k_sp_rel || info.b == k_sp_rel || info.c == k_sp_rel) return false;
    return true;
}

static bool writes_place(const ir_instruction &instr, const place &where) {
    const instruction_info &info = instruction_info_of(instr.op);

    if(where.kind == place_register) {
        register_set reads, writes;
        register_effects(instr, reads, writes);
        return (writes >> where.value) & 1u;
    }

    if(where.kind == place_local) {
        const operand_kind kinds[3] = { info.a, info.b, info.c };
        const int32_t values[3] = { instr.a, instr.b, instr.c };
        for(int k = 0; k < 3; k++) {
            if((kinds[k] == k_var_w || kinds[k] == k_var_rw) && values[k] == where.value) return true;
        }
    }

    return false;
}

/**
 * Is the value in `reg` just before ir.code[index] certain to be exactly
 * representable as a double? Looks at the instruction in the same block that
 * last wrote it.
 */
static bool register_holds_double(function_ir &ir, control_flow &cfg, int index, int reg) {
    int first = cfg.blocks[cfg.blockOf[index]].first;

    for(int i = index - 1; i >= first; i--) {
        ir_instruction &instr = ir.code[i];
        register_set reads, writes;
        register_effects(instr, reads, writes);
        if(!((writes >> reg) & 1u)) continue;

        switch(instr.op) {
            case Opcode::LOADL: case Opcode::LOADVAL: case Opcode::MOVI: case Opcode::LDC: case Opcode::SMOV:
            case Opcode::LT: case Opcode::GT: case Opcode::LTE: case Opcode::GTE:
            case Opcode::TEST: case Opcode::TNE: case Opcode::NOT: case Opcode::CMP:
            case Opcode::AND: case Opcode::CHECKNULL: case Opcode::SIZEOF: case Opcode::ITEST: case Opcode::IS:
            case Opcode::MOV8: case Opcode::MOV16: case Opcode::MOV32:
            case Opcode::MOVU8: case Opcode::MOVU16: case Opcode::MOVU32:
                return true;
            default:
                return false;
        }
    }

    return false; // written in another block: unknown
}

/** One direct move from `source` to `destination`, or false if there is no instruction for it */
static bool make_move(function_ir &ir, const place &source, const place &destination, ir_instruction &move, bool &nothingNeeded) {
    nothingNeeded = false;

    if(destination.kind == place_register) {
        switch(source.kind) {
            case place_constant: move = make_instruction(ir, Opcode::MOVI, destination.value, source.value); return true;
            case place_local:    move = make_instruction(ir, Opcode::LOADL, destination.value, source.value); return true;
            case place_register:
                if(source.value == destination.value) { nothingNeeded = true; return true; }
                move = make_instruction(ir, Opcode::MOVR, destination.value, source.value);
                return true;
            default: return false;
        }
    }

    if(destination.kind == place_local) {
        switch(source.kind) {
            case place_constant: move = make_instruction(ir, Opcode::ISTOREL, destination.value, source.value); return true;
            case place_register: move = make_instruction(ir, Opcode::SMOVR_2, source.value, destination.value); return true;
            case place_local:
                if(source.value == destination.value) { nothingNeeded = true; return true; }
                move = make_instruction(ir, Opcode::SMOVR_4, destination.value, source.value);
                return true;
            default: return false;
        }
    }

    return false;
}

/** Finds and rewrites one pair. Returns true if it changed the function. */
static bool forward_one(function_ir &ir) {
    control_flow cfg;
    liveness live;
    build_control_flow(ir, cfg);
    compute_liveness(ir, cfg, live);

    for(size_t popIndex = 0; popIndex < ir.code.size(); popIndex++) {
        place destination = popped_into(ir.code[popIndex]);
        if(destination.kind == place_none) continue;
        if(destination.kind == place_local && !live.localsKnown) continue;

        // ---- walk back to the push that produced the popped value
        int first = cfg.blocks[cfg.blockOf[popIndex]].first;
        int pushIndex = -1;
        for(int i = (int)popIndex - 1; i >= first; i--) {
            if(pushed_from(ir.code[i]).kind != place_none) { pushIndex = i; break; }
            if(!leaves_stack_alone(ir.code[i])) break;
        }
        if(pushIndex < 0) continue;

        place source = pushed_from(ir.code[pushIndex]);

        // ---- 1. nobody reads what the pop stores: drop the pair
        bool destinationDead = destination.kind == place_register
                ? !live.register_live_after(popIndex, destination.value)
                : !live.local_live_after(popIndex, destination.value);

        if(destinationDead) {
            remove_instruction(ir, popIndex);
            remove_instruction(ir, (size_t)pushIndex);
            return true;
        }

        // ---- 2. replace the pair with a direct move
        bool sourceIntact = true;
        for(int i = pushIndex + 1; i < (int)popIndex && sourceIntact; i++)
            sourceIntact = !writes_place(ir.code[i], source);
        if(!sourceIntact) continue;

        if(source.kind == place_register && destination.kind == place_register
           && source.value != destination.value
           && !register_holds_double(ir, cfg, pushIndex, source.value))
            continue; // see NOTE ON PRECISION

        if(source.kind == place_local && !live.localsKnown) continue;

        ir_instruction move;
        bool nothingNeeded;
        if(!make_move(ir, source, destination, move, nothingNeeded)) continue;

        if(nothingNeeded) {
            // pushing a place and popping it back into the same place: for a
            // register that would round it to double, so only when it is exact
            if(source.kind == place_register && !register_holds_double(ir, cfg, pushIndex, source.value)) continue;
            remove_instruction(ir, popIndex);
        } else {
            // the move takes the place (and the identity) of the pop
            move.id = ir.code[popIndex].id;
            ir.nextId--;                 // give back the id make_instruction reserved
            ir.forward.pop_back();
            ir.code[popIndex] = move;
        }

        remove_instruction(ir, (size_t)pushIndex);
        return true;
    }

    return false;
}

bool pass_stack_forwarding(function_ir &ir) {
    bool changed = false;
    while(forward_one(ir)) changed = true;
    return changed;
}
