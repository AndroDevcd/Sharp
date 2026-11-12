//
// Sharp bytecode optimizer - the list of passes
//
// Each pass is one function `bool pass_xxx(function_ir &ir)` in its own file in
// this folder. It returns true if it changed the function. To add one, declare
// it here and add a line to OPTIMIZER_PASS_LIST:
//
//     X("name shown in -Oreport", lowest -O level, function)
//
// Order matters only for speed: the driver repeats the whole list until no
// pass changes anything.
//

#ifndef SHARP_OPT_PASSES_H
#define SHARP_OPT_PASSES_H

#include "../function_ir.h"
#include "../analysis/box_constructors.h"
#include <map>

/** Every function the optimizer could decode, for the passes that look across functions */
struct program_ir {
    std::vector<function_ir*> functions;
    std::map<Int, function_ir*> byAddress;   // function address (the operand of `call`) -> function
    Int callsInlined = 0;
    Int boxesRemoved = 0;
    Int loopsUnrolled = 0;
    Int loopsRotated = 0;
    Int loopsInRegisters = 0;
    std::map<Int, box_summary> boxSummaries;  // cache of analysis/box_constructors.h, by function address
};

/** push/pop pairs on the operand stack become direct moves, or disappear   (pass_stack_forwarding.cpp) */
bool pass_stack_forwarding(function_ir &ir);
/** values known at compile time are computed at compile time, branches on
 *  known conditions are decided                                             (pass_constant_propagation.cpp) */
bool pass_constant_propagation(function_ir &ir);
/** register-to-register moves are folded into the instructions around them (pass_copy_propagation.cpp) */
bool pass_copy_propagation(function_ir &ir);
/** computations and variables whose value is never read are removed        (pass_dead_stores.cpp) */
bool pass_dead_stores(function_ir &ir);
/** everything that contributes to nothing observable: self-feeding variables,
 *  ifs that guard nothing, whole loops whose results are unused            (pass_dead_code.cpp) */
bool pass_dead_code(function_ir &ir);
/** jumps to the next instruction and jumps to jumps                        (pass_jump_cleanup.cpp) */
bool pass_jump_cleanup(function_ir &ir);
/** code that can never run                                                 (pass_unreachable_code.cpp) */
bool pass_unreachable_code(function_ir &ir);

// ---- loop shaping, run by the driver once after everything else (at -O2 and up), in this order
/** a loop with a small known trip count becomes straight-line code (-O3)   (pass_loop_unrolling.cpp) */
bool pass_loop_unrolling(function_ir &ir);
/** the loop test moves to the bottom of the loop: one jump per trip instead of two  (pass_loop_rotation.cpp) */
bool pass_loop_rotation(function_ir &ir);
/** the number locals a loop works with live in registers for the duration of the loop (pass_loop_registers.cpp) */
bool pass_loop_registers(function_ir &ir);

// ---- passes that work across functions (run by the driver at -O3, not part of the list below) ----
/** small callees are copied into their callers                             (pass_inline_calls.cpp) */
bool pass_inline_calls(function_ir &caller, program_ir &program);
/** number objects that never leave the function are replaced by a plain local (pass_box_elimination.cpp) */
bool pass_box_elimination(function_ir &ir, program_ir &program);

#define OPTIMIZER_PASS_LIST(X) \
    X("stack forwarding", 1, pass_stack_forwarding) \
    X("copy propagation", 1, pass_copy_propagation) \
    X("constant propagation", 2, pass_constant_propagation) \
    X("dead stores",      1, pass_dead_stores) \
    X("dead code",        2, pass_dead_code) \
    X("jump clean-up",    1, pass_jump_cleanup) \
    X("unreachable code", 1, pass_unreachable_code)

#endif //SHARP_OPT_PASSES_H
