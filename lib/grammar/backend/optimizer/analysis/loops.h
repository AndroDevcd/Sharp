//
// Sharp bytecode optimizer - finding the loops of a function
// ==========================================================
//
// The bytecode has no "for" or "while" any more, only jumps. A loop is found
// the way every optimizing compiler finds it:
//
//   - block D DOMINATES block B if every path from the function entry to B
//     goes through D
//   - a jump from B back to a block H that dominates B is a BACK EDGE, and H is
//     the HEADER of a loop (the place the loop is entered and re-entered)
//   - the loop's body is every block that can reach B without going through H
//
//       source                      blocks
//       ------------------------    ---------------------------------------------
//       for(i = 0; i < n; i++) {    entry:   i = 0
//           total += i;             header:  if(!(i < n)) goto exit   <-+
//       }                           body:    total += i; i++;           |
//                                            jmp header   --------------+  back edge
//                                   exit:    ...
//
// The loop passes (passes/pass_loop_*.cpp) work on this description.
//
// `valid` is false, and no loops are reported, for functions the analysis does
// not handle: try/catch/finally (exception edges), jumps whose target is only
// known at run time, unreachable blocks (run the unreachable code pass first).
//

#ifndef SHARP_OPT_LOOPS_H
#define SHARP_OPT_LOOPS_H

#include "control_flow.h"

struct loop_info {
    int header;                     // block every entry into the loop goes through
    std::vector<int> blocks;        // all blocks of the loop, header included, ascending
    std::vector<char> contains;     // contains[b] = block b is part of the loop
    std::vector<int> latches;       // blocks that jump back to the header
    std::vector<int> entries;       // blocks outside the loop that lead to the header
    std::vector<int> exits;         // blocks outside the loop that a block inside leads to
};

struct loop_forest {
    bool valid = false;
    std::vector<loop_info> loops;   // smallest (innermost) first
};

void find_loops(function_ir &ir, control_flow &cfg, loop_forest &forest);

#endif //SHARP_OPT_LOOPS_H
