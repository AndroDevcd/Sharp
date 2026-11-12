//
// Pass: object copies   (-O)
// ==========================
// Removes copies of an object reference from one local slot to another when
// the copy is only ever used while the original still holds the same object.
//
// WHY IT MATTERS
//   Copying an object reference is not free: the VM updates the object's
//   reference count (an atomic add, and an atomic subtract when the slot is
//   overwritten later). Inlining produces a lot of these copies, because every
//   inlined function gets its own slot for `self`:
//
//       p := new point(i, 1);          constructor chain point -> _object_
//
//       after inlining, per object created      after this pass
//       -------------------------------------   ------------------------------
//         popl 3            ; the new object      popl 3
//         movl 3
//         pushobj
//         popl 6            ; point's self        (gone)
//         ...
//         movl 6
//         pushobj
//         popl 8            ; _object_'s self     (gone)
//         ...
//         movl 6            ; self.x = ...        movl 3
//         movn #0                                 movn #0
//
//   Without this pass an allocation was SLOWER at -O than without it.
//
// HOW
//   A copy is one of
//         pushl a ; popl b                 b = a
//         movl a ; pushobj ; popl b        b = a
//   and it is a candidate if that `popl b` is the only instruction in the
//   function that writes b.
//
//   A forward data flow analysis then answers, for every point of the function
//   and every candidate: "is b still certain to hold the same object as a?"
//     - it becomes true at the copy
//     - it becomes false when a (or b) is written, when something is written
//       through ptr (popobj / del: we do not track where ptr points), and at
//       anything opaque
//     - where paths join it is true only if it is true on every path
//
//   Every use of b at a point where the answer is "yes" is changed to use a.
//   If no use of b is left afterwards, the copy itself is deleted.
//
//   `movl b` (take the address of the slot) is only redirected to a when what
//   follows reads through ptr. If it WRITES through ptr the two slots are not
//   interchangeable: the code means to change b, not a.
//
// LIMITS
//   Functions with try/catch/finally are skipped; at most 64 copies per function.
//

#include "passes.h"
#include "../analysis/control_flow.h"
#include <algorithm>

#define MAX_COPIES 64

namespace {

struct object_copy {
    int from, to;       // b(to) = a(from)
    int popIndex;       // index of the `popl b`
    int firstIndex;     // first instruction that can be deleted with it (the pushl, or the pushobj)
};

typedef uint64_t copy_set;

/** Does the code after the `movl` at `index` only READ through ptr before ptr is set again? */
bool only_reads_through_pointer(function_ir &ir, control_flow &cfg, int index) {
    int last = cfg.blocks[cfg.blockOf[index]].last;

    for(int i = index + 1; i <= last; i++) {
        const ir_instruction &instr = ir.code[i];
        const instruction_info &info = instruction_info_of(instr.op);

        if(instr.op == Opcode::POPOBJ || instr.op == Opcode::DEL) return false;     // *ptr = ...
        if(info.flags & (f_call | f_opaque)) return false;                           // ptr is handed on
        if((info.flags & f_writes_ptr) && !(info.flags & f_reads_ptr)) return true;  // ptr set to something new
        // movn / movnd move ptr into the object: from here on ptr no longer points at the slot
        if(instr.op == Opcode::MOVN || instr.op == Opcode::MOVND) return true;
    }

    return false;   // still pointing at the slot when the block ends: do not guess
}

/** state after one instruction */
void transfer(function_ir &ir, const std::vector<object_copy> &copies, int index, copy_set &valid) {
    const ir_instruction &instr = ir.code[index];
    const instruction_info &info = instruction_info_of(instr.op);

    if((info.flags & f_opaque) || instr.op == Opcode::POPOBJ || instr.op == Opcode::DEL) {
        valid = 0;
        return;
    }

    const operand_kind kinds[3] = { info.a, info.b, info.c };
    const int32_t values[3] = { instr.a, instr.b, instr.c };
    for(int k = 0; k < 3; k++) {
        if(kinds[k] != k_obj_w) continue;
        for(size_t c = 0; c < copies.size(); c++) {
            if(copies[c].from == values[k] || copies[c].to == values[k]) valid &= ~((copy_set)1 << c);
        }
    }

    for(size_t c = 0; c < copies.size(); c++) {
        if(copies[c].popIndex == index) valid |= (copy_set)1 << c;
    }
}

} // namespace

bool pass_object_copies(function_ir &ir) {
    if(!ir.failure.empty() || !ir.tries.empty()) return false;

    control_flow cfg;
    build_control_flow(ir, cfg);
    int count = (int)ir.code.size();
    int slots = ir.frameSlots > 0 ? ir.frameSlots : 1;

    // ---- how often is each object slot written?
    std::vector<int> writes(slots, 0);
    for(const ir_instruction &instr : ir.code) {
        const instruction_info &info = instruction_info_of(instr.op);
        const operand_kind kinds[3] = { info.a, info.b, info.c };
        const int32_t values[3] = { instr.a, instr.b, instr.c };
        for(int k = 0; k < 3; k++) {
            if(!is_local_kind(kinds[k])) continue;
            if(values[k] < 0 || values[k] >= slots) return false;   // a slot we do not understand
            if(kinds[k] == k_obj_w) writes[values[k]]++;
        }
    }

    // ---- the candidates
    std::vector<object_copy> copies;
    for(int i = 1; i < count && copies.size() < MAX_COPIES; i++) {
        const ir_instruction &pop = ir.code[i];
        if(pop.op != Opcode::POPL || writes[pop.a] != 1) continue;
        if(cfg.blockOf[i] != cfg.blockOf[i - 1]) continue;

        object_copy copy;
        copy.to = pop.a;
        copy.popIndex = i;

        const ir_instruction &before = ir.code[i - 1];
        if(before.op == Opcode::PUSHL) {                                   // pushl a ; popl b
            copy.from = before.a;
            copy.firstIndex = i - 1;
        } else if(before.op == Opcode::PUSHOBJ && i >= 2 && cfg.blockOf[i - 2] == cfg.blockOf[i]
                  && ir.code[i - 2].op == Opcode::MOVL) {                  // movl a ; pushobj ; popl b
            copy.from = ir.code[i - 2].a;
            copy.firstIndex = i - 1;
        } else continue;

        if(copy.from == copy.to) continue;
        if(copy.to == 0) continue;     // slot 0 is what the caller reads after `ret`
        copies.push_back(copy);
    }
    if(copies.empty()) return false;

    // ---- "b still equals a": forward, true only if true on every path
    size_t blocks = cfg.blocks.size();
    copy_set everything = copies.size() == 64 ? ~(copy_set)0 : (((copy_set)1 << copies.size()) - 1);
    std::vector<copy_set> in(blocks, everything), out(blocks, everything);
    in[0] = 0;

    bool changed = true;
    while(changed) {
        changed = false;
        for(size_t b = 0; b < blocks; b++) {
            if(!cfg.blocks[b].reachable) continue;

            copy_set state = b == 0 ? 0 : everything;
            if(b != 0) {
                for(int pred : cfg.blocks[b].predecessors) {
                    if(cfg.blocks[pred].reachable) state &= out[pred];
                }
            }
            in[b] = state;

            for(int i = cfg.blocks[b].first; i <= cfg.blocks[b].last; i++)
                transfer(ir, copies, i, state);

            if(state != out[b]) { out[b] = state; changed = true; }
        }
    }

    // ---- redirect the uses
    bool rewrote = false;
    for(size_t b = 0; b < blocks; b++) {
        if(!cfg.blocks[b].reachable) continue;
        copy_set state = in[b];

        for(int i = cfg.blocks[b].first; i <= cfg.blocks[b].last; i++) {
            ir_instruction &instr = ir.code[i];
            const instruction_info &info = instruction_info_of(instr.op);
            const operand_kind kinds[3] = { info.a, info.b, info.c };
            int32_t *values[3] = { &instr.a, &instr.b, &instr.c };

            for(int k = 0; k < 3; k++) {
                if(kinds[k] != k_obj_r && kinds[k] != k_slot_addr) continue;

                for(size_t c = 0; c < copies.size(); c++) {
                    if(*values[k] != copies[c].to || !((state >> c) & 1u)) continue;
                    if(kinds[k] == k_slot_addr && !only_reads_through_pointer(ir, cfg, i)) continue;

                    *values[k] = copies[c].from;
                    instr.modified = true;
                    rewrote = true;
                    break;
                }
            }

            transfer(ir, copies, i, state);
        }
    }

    // ---- delete the copies nobody reads any more
    std::vector<int> uses(slots, 0);
    for(const ir_instruction &instr : ir.code) {
        const instruction_info &info = instruction_info_of(instr.op);
        if(info.flags & f_opaque) return rewrote;   // may look at any slot: keep every copy
        const operand_kind kinds[3] = { info.a, info.b, info.c };
        const int32_t values[3] = { instr.a, instr.b, instr.c };
        for(int k = 0; k < 3; k++) {
            if(kinds[k] == k_obj_r || kinds[k] == k_slot_addr) uses[values[k]]++;
        }
    }

    std::vector<int> remove;
    for(const object_copy &copy : copies) {
        if(uses[copy.to] != 0) continue;
        remove.push_back(copy.firstIndex);      // pushl a   /  pushobj
        remove.push_back(copy.popIndex);        // popl b
    }
    std::sort(remove.begin(), remove.end());
    for(size_t i = remove.size(); i-- > 0; ) {
        remove_instruction(ir, (size_t)remove[i]);
        rewrote = true;
    }

    return rewrote;
}
