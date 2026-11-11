//
// Sharp bytecode optimizer - control flow graph
// =============================================
//
// Splits a function into BASIC BLOCKS: runs of instructions that are always
// executed together (control enters only at the first one and leaves only at
// the last one), and records which block can follow which.
//
// Besides ordinary jumps there are two hidden ways control moves, and both are
// part of the graph:
//
//   * EXCEPTIONS. Any instruction inside a try block may jump to that try's
//     catch handlers or finally block. A block that lies inside a protected
//     range lists those entry points in `handlers`.
//
//   * INDIRECT JUMPS. `brh` jumps to an address held in a register. The only
//     addresses the generator ever puts there are the tagged code-address
//     constants (function_ir.h), so `brh` may go to any of those.
//
// The graph describes ir.code as it is right now. Build it again after a pass
// has changed the function.
//

#ifndef SHARP_OPT_CONTROL_FLOW_H
#define SHARP_OPT_CONTROL_FLOW_H

#include "../function_ir.h"

struct basic_block {
    int first, last;                // indexes into ir.code, both inclusive
    std::vector<int> successors;    // blocks that may run next through normal control flow
    std::vector<int> handlers;      // blocks an exception raised in this block may continue at
    std::vector<int> predecessors;  // blocks with this one in successors or handlers
    bool reachable;                 // can be reached from the function entry at all
    bool isHandler;                 // entered by the runtime when an exception is caught (catch / finally entry)
};

struct control_flow {
    std::vector<basic_block> blocks;    // blocks[0] is the function entry
    std::vector<int> blockOf;           // blockOf[i] = block containing ir.code[i]
};

void build_control_flow(function_ir &ir, control_flow &cfg);

#endif //SHARP_OPT_CONTROL_FLOW_H
