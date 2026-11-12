//
// Converting a constant between integer and decimal while compiling, instead
// of emitting an instruction that does it every time the code runs.
//
// The compiler knows the kind of every number (integer or decimal) and puts a
// conversion wherever a value changes kind (compiler/expressions/
// numeric_conversion.h). When the value is a literal, that used to become
//
//      ch >= 'A'       (ch: decimal)       loadl cx, fp+0
//                                          movi  #65, ecx      ; the integer 65
//                                          i2d   ecx, ecx      ; ...made a decimal, at run time
//                                          dgte  cx, ecx
//
// With this file the constant is loaded as the right kind to begin with:
//
//                                          loadl cx, fp+0
//                                          dmovi #65, ecx      ; the decimal 65.0
//                                          dgte  cx, ecx
//
// C++ comparison: `x >= 65` with a double x - the C++ compiler stores 65.0 in
// the program, it does not convert the int 65 on every comparison.
//

#ifndef SHARP_CONSTANT_CONVERSION_H
#define SHARP_CONSTANT_CONVERSION_H

#include "../../../operation/operation.h"
#include "../../../../../core/opcode/opcode.h"

/**
 * Called when register `r` is about to be converted. If the instruction just
 * emitted loaded a constant into `r`, that instruction is replaced by one that
 * loads the converted constant, and true is returned: the caller emits nothing.
 *
 *      movi  k, r      to decimal      dmovi k, r
 *      dmovi k, r      to integer      movi  k, r
 *      ldc   r, c      to integer      movi  k, r      when c is a whole number that fits
 *      movi / dmovi / ldc              to "is it not zero"     movi 0 or 1, r
 *
 * Returns false (and changes nothing) when the last instruction is anything
 * else, or when code can jump to this point from somewhere else - then the
 * register may hold a value that did not come from that instruction.
 */
bool fold_constant_conversion(_register r, numeric_conversion conversion);

#endif //SHARP_CONSTANT_CONVERSION_H
