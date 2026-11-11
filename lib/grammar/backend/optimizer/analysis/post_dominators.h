//
// Sharp bytecode optimizer - post-dominators and control dependence
// =================================================================
//
// Two facts about the shape of a function that the "remove whole loops / whole
// ifs" optimization (passes/pass_dead_code.cpp) is built on.
//
// POST-DOMINATOR
//   Block P post-dominates block X if every way of getting from X to the end of
//   the function passes through P. The closest such P is X's IMMEDIATE
//   post-dominator: "the place where control comes back together after X, no
//   matter which way X branches".
//
//       X:  if(c) { A } else { B }        immediate post-dominator of X = J
//       J:  ...
//
//       H:  while(c) { BODY }             immediate post-dominator of H = E
//       E:  ...                           (the loop exit)
//
// CONTROL DEPENDENCE
//   Block Y is control dependent on block X if X ends in a branch and that
//   branch decides whether Y runs: one way out of X always reaches Y, another
//   way can avoid it. In the examples above A and B are control dependent on X,
//   and BODY (and H itself, through the back edge) on H.
//
// Why this matters: a branch is only worth keeping if something that has to
// stay in the program is control dependent on it. If nothing is, the branch and
// everything it guards can be replaced by a jump to the immediate
// post-dominator.
//
// LIMITS
//   Only computed for functions where every block can reach the end of the
//   function through normal control flow and there are no exception handlers
//   or indirect jumps (`valid` is false otherwise, and the caller must not use
//   the results).
//

#ifndef SHARP_OPT_POST_DOMINATORS_H
#define SHARP_OPT_POST_DOMINATORS_H

#include "control_flow.h"

/** "the end of the function" as a post-dominator */
#define PDOM_EXIT (-1)

struct post_dominators {
    bool valid;
    /** immediate post-dominator of each block: a block index, or PDOM_EXIT */
    std::vector<int> immediate;
    /** dependsOn[y] = blocks whose final branch decides whether block y runs */
    std::vector<std::vector<int>> dependsOn;
};

void compute_post_dominators(function_ir &ir, control_flow &cfg, post_dominators &pdom);

#endif //SHARP_OPT_POST_DOMINATORS_H
