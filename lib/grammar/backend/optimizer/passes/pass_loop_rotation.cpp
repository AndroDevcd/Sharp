//
// Pass: loop rotation   (-O2, run once after all other passes)
// ===========================================================
// Moves the test of a loop from the top to the bottom, so that every trip
// around the loop executes one jump instead of two.
//
//     for(i = 0; i < n; i++) { total += i; }
//
//     before                               after
//     ---------------------------------    ---------------------------------
//             istorel 0, i                         istorel 0, i
//       top:  loadl cx, i      <--+                loadl cx, i         ; first test,
//             loadl ecx, n        |                loadl ecx, n        ; runs once
//             lt cx, ecx          |                lt cx, ecx
//             jne exit            |                jne exit
//             ... total += i      |          body: ... total += i   <--+
//             iaddl 1, i          |                iaddl 1, i          |
//             jmp top    ---------+                loadl cx, i         |  the test again,
//       exit:                                      loadl ecx, n        |  at the bottom
//                                                  lt cx, ecx          |
//                                                  je body   ----------+
//                                            exit:
//
//   Per trip: before = test + `jne` (not taken) + body + `jmp`;
//             after  = body + test + `je`. One instruction less on every trip,
//   and in compiled code one jump less. The cost is a second copy of the test.
//
// WHY THIS IS ALWAYS CORRECT
//   The `jmp top` is replaced by a copy of exactly the instructions it used to
//   jump to (the header block), so the same instructions run in the same order
//   as before. Only the final conditional jump is turned around: "leave if
//   false, else fall into the body" becomes "back to the body if true, else
//   fall out" - and the instruction after the old `jmp` is the loop exit.
//
// WHEN IT APPLIES
//   - the loop header ends in `je` / `jne` that leaves the loop, and execution
//     falls through into the body
//   - a block of the loop ends in `jmp header` and is directly followed by the
//     loop exit (this is how the generator lays out for / while loops)
//   - the header is short (MAX_TEST_INSTRUCTIONS), since it is duplicated
//   Other jumps back to the header (`continue`) are left as they are.
//
// THE VM SIDE
//   The interpreter starts the JIT for a hot loop when it sees a backward jump.
//   After rotation the backward jump is a conditional one, so JE / JNE count
//   too (loop_back_edge in lib/runtime/virtual_machine.cpp).
//

#include "passes.h"
#include "../analysis/control_flow.h"
#include "../analysis/loops.h"

/* a longer loop test is not duplicated */
#define MAX_TEST_INSTRUCTIONS 10
#define MAX_FUNCTION_INSTRUCTIONS 6000

/** Rotate one loop. Returns true if the function was changed (indexes are stale afterwards). */
static bool rotate_one(function_ir &ir) {
    control_flow cfg;
    loop_forest forest;
    build_control_flow(ir, cfg);
    find_loops(ir, cfg, forest);
    if(!forest.valid) return false;

    for(loop_info &loop : forest.loops) {
        basic_block &header = cfg.blocks[loop.header];
        ir_instruction &test = ir.code[header.last];

        // ---- the header: "... ; je/jne exit", falling through into the body
        if(test.op != Opcode::JE && test.op != Opcode::JNE) continue;
        if(header.last - header.first + 1 > MAX_TEST_INSTRUCTIONS) continue;
        if(header.last + 1 >= (int)ir.code.size()) continue;

        int exitId = resolve_reference(ir, test.target);
        int exitIndex = exitId >= 0 ? index_of(ir, exitId) : -1;
        if(exitIndex < 0) continue;

        int exitBlock = cfg.blocks.empty() ? -1 : cfg.blockOf[exitIndex];
        int bodyBlock = cfg.blockOf[header.last + 1];
        if(loop.contains[exitBlock] || !loop.contains[bodyBlock]) continue;
        if(cfg.blocks[exitBlock].first != exitIndex) continue;

        bool copyable = true;
        for(int i = header.first; i < header.last && copyable; i++) {
            const ir_instruction &instr = ir.code[i];
            // one block: nothing in it jumps, but be sure nothing in it names a code address
            copyable = instr.target == IR_NONE && !instr.pcConstant;
        }
        if(!copyable) continue;

        // ---- the bottom: "jmp header", directly followed by the exit
        for(int latch : loop.latches) {
            if(latch == loop.header) continue;

            int jumpIndex = cfg.blocks[latch].last;
            ir_instruction &jump = ir.code[jumpIndex];
            if(jump.op != Opcode::JMP) continue;
            if(jumpIndex + 1 != exitIndex) continue;

            int targetId = resolve_reference(ir, jump.target);
            if(targetId < 0 || index_of(ir, targetId) != header.first) continue;

            // ---- build the copy of the header that takes the place of the jmp
            std::vector<ir_instruction> bottom;
            for(int i = header.first; i < header.last; i++) {
                const ir_instruction &original = ir.code[i];
                bottom.push_back(make_instruction(ir, original.op, original.a, original.b, original.c));
            }

            // the test, turned around: stay in the loop when the condition says so
            ir_instruction back = make_instruction(ir, test.op == Opcode::JE ? Opcode::JNE : Opcode::JE, 0);
            back.target = ir.code[header.last + 1].id;      // first instruction of the body
            bottom.push_back(back);

            // anything that pointed at the jmp now points at the start of the copy
            int jumpId = jump.id;
            ir.forward[jumpId] = bottom.front().id;
            ir.code.erase(ir.code.begin() + jumpIndex);
            ir.code.insert(ir.code.begin() + jumpIndex, bottom.begin(), bottom.end());

            ir.changed = true;
            return true;
        }
    }

    return false;
}

bool pass_loop_rotation(function_ir &ir) {
    if(!ir.failure.empty() || !ir.tries.empty()) return false;

    bool changed = false;
    // each call rotates one loop and starts over, because the blocks moved
    for(int round = 0; round < 64 && ir.code.size() < MAX_FUNCTION_INSTRUCTIONS && rotate_one(ir); round++)
        changed = true;

    return changed;
}
