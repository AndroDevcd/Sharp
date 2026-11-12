//
// Sharp bytecode optimizer - driver
// =================================
//
// WHERE IT RUNS
//   generate() (finalizer/generation/generator.cpp) first lets the code
//   generator produce every function, then calls optimize_bytecode(), then
//   writes the .aout listing and the executable. So the optimizer sees finished
//   bytecode for the whole program, and the .aout shows what it produced.
//
// LEVELS (sharpc -O0 .. -O3, default -O0)
//   -O0  the optimizer does not run at all
//   -O1  local clean-up inside a function: no instruction is ever reordered
//        across something observable
//   -O2  + analysis across the whole function
//   -O3  + optimizations across functions (inlining ...)
//   Every pass states the level it belongs to where it is registered in
//   bytecode_optimizer.cpp.
//
// SAFETY NET
//   For every function, before any pass runs:
//     1. it is decoded (function_ir.h). Functions the optimizer does not fully
//        understand are skipped and emitted untouched.
//     2. it is re-encoded from the decoded operands and compared with the
//        original. Any difference means decode/encode disagree with the code
//        generator, and the function is skipped.
//   After the passes ran, the result is decoded once more as a sanity check. If
//   that fails the function is restored to exactly what the generator produced.
//
// HOW TO ADD A PASS
//   1. write   bool pass_xxx(function_ir &ir)   in passes/pass_xxx.cpp. It
//      edits ir.code using the helpers in function_ir.h and returns true if it
//      changed anything. Use instruction_info_of() for every question about an
//      opcode. Put a before/after bytecode example in the file comment.
//   2. declare it in passes/passes.h and add one line to the pass list in
//      bytecode_optimizer.cpp with its name and level.
//   `sharpc -Oreport` then shows how many instructions it removed.
//

#ifndef SHARP_BYTECODE_OPTIMIZER_H
#define SHARP_BYTECODE_OPTIMIZER_H

#include <string>
#include <vector>

/** Optimize every generated function (does nothing unless options.optimize_level is set by -O) */
void optimize_bytecode();

/**
 * The name of every optimization, as -Oreport prints it ("loop rotation").
 * On the command line the same name with dashes switches it off:
 *     sharpc -O -nloop-rotation -ninline-calls ...
 */
const std::vector<std::string>& optimization_names();

#endif //SHARP_BYTECODE_OPTIMIZER_H
