//
// Pass: jump clean-up
// ===================
// Two simplifications of jumps whose target is known at compile time
// (jmp / je / jne):
//
// 1. A jump to the very next instruction does nothing.
//
//        jmp  @next          ->      (removed)
//      next:
//
//    A conditional jump (je / jne) to the next instruction is removed too:
//    whether or not the condition holds, the next instruction runs.
//
// 2. A jump whose target is itself an unconditional `jmp` goes straight to the
//    final destination ("jump threading").
//
//        jne  @a             ->      jne  @b
//        ...                         ...
//      a:                          a:
//        jmp  @b                     jmp  @b      (removed later if nothing else jumps to it)
//

#include "passes.h"

static bool is_direct_jump(const ir_instruction &instr) {
    return instr.op == Opcode::JMP || instr.op == Opcode::JE || instr.op == Opcode::JNE;
}

bool pass_jump_cleanup(function_ir &ir) {
    bool changedAtAll = false;
    bool changed = true;

    while(changed) {
        changed = false;

        for(size_t i = 0; i < ir.code.size(); i++) {
            ir_instruction &instr = ir.code[i];
            if(!is_direct_jump(instr)) continue;

            int target = resolve_reference(ir, instr.target);

            // ---- 2. follow chains of unconditional jumps (bounded, in case of a jump cycle)
            for(int hops = 0; hops < 8 && target != IR_END; hops++) {
                ir_instruction &destination = ir.code[index_of(ir, target)];
                if(destination.op != Opcode::JMP || destination.id == instr.id) break;

                int next = resolve_reference(ir, destination.target);
                if(next == target) break; // a jump to itself: an endless loop, leave it
                target = next;
            }

            if(target != resolve_reference(ir, instr.target)) {
                instr.target = target;
                instr.modified = true;
                changed = true;
            }

            // ---- 1. jump to the next instruction
            int nextId = i + 1 < ir.code.size() ? ir.code[i + 1].id : IR_END;
            if(target != IR_END && target == nextId) {
                remove_instruction(ir, i);
                changed = true;
                i--;
            }
        }

        changedAtAll |= changed;
    }

    return changedAtAll;
}
