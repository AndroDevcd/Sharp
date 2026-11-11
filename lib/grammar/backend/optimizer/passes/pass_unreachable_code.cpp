//
// Pass: unreachable code
// ======================
// Removes instructions that can never run: nothing jumps to them, nothing falls
// into them, and they are not a catch/finally entry of a reachable try block.
//
// Mostly this cleans up after other passes. Once a condition is known to be
// constant, the branch that can no longer be taken is unreachable, and deleting
// it is what makes whole `if` bodies and loops disappear.
//
//     before                         after
//     -------------------------      -------------------------
//       jmp  @done                     jmp  @done
//       movi 1, ebx        <- dead   done:
//       rstore ebx         <- dead     ret
//     done:
//       ret
//

#include "passes.h"
#include "../analysis/control_flow.h"

bool pass_unreachable_code(function_ir &ir) {
    control_flow cfg;
    build_control_flow(ir, cfg);

    bool changed = false;

    // back to front so the indexes of blocks we have not handled yet stay valid
    for(size_t b = cfg.blocks.size(); b-- > 0; ) {
        basic_block &block = cfg.blocks[b];
        if(block.reachable) continue;

        for(int i = block.last; i >= block.first; i--)
            remove_instruction(ir, (size_t)i);
        changed = true;
    }

    return changed;
}
