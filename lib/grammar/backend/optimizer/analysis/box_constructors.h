//
// Sharp bytecode optimizer - what does a call do to a freshly created object?
// ==========================================================================
//
// Every number that crosses a function boundary in Sharp is "boxed": wrapped in
// a small object of one of the std number classes (std#int, std#double, ...).
//
//     def square(x: var): double { return x * x; }
//
//         new_class std#double        ; allocate the box
//         dup
//         ... x * x -> bmr
//         rstore bmr                  ; push the number
//         call std#double.double      ; the constructor: self.value = number
//
// To remove a box (passes/pass_box_elimination.cpp) the optimizer must know
// what that constructor call does to the object. Nothing about the std classes
// is hard coded: the constructor's bytecode is EXECUTED SYMBOLICALLY here, calls
// included, and it qualifies only if the one and only thing it does is
//
//     self.<field 0> = <optional integer casts>(the number argument)
//
// (`stores_value`), or nothing at all (`no_effect`, e.g. the empty init<T>()
// functions). Any instruction the executor does not understand, any other heap
// write, anything that lets `self` get away -> `unknown`, and the box stays.
// So this works for any user class that is a plain one-number wrapper too, and
// it stops working by itself (instead of miscompiling) if a std constructor is
// ever changed to do more.
//
// ONE ASSUMPTION
//   Constructors start with "run the class's static initializer if it has not
//   run yet" (a hidden static flag, field 0 of the class's static object):
//
//       movg  @Class ; movn #0 ; iload cmt ; je <skip> ; call static_init<Class>
//
//   The executor takes the flag to be SET. That is what happens at run time:
//   the program's start-up function static_init() calls the static initializer
//   of every class before any user code runs.
//

#ifndef SHARP_OPT_BOX_CONSTRUCTORS_H
#define SHARP_OPT_BOX_CONSTRUCTORS_H

#include "../function_ir.h"
#include <map>
#include <vector>

struct program_ir;

enum box_effect : uint8_t {
    box_unknown,        // not understood: the object must be treated as escaping
    box_no_effect,      // f(self): does nothing observable
    box_stores_value    // f(self, number): self.field[0] = casts(number), nothing else
};

struct box_summary {
    box_effect effect = box_unknown;
    /** the MOV8 / MOVU16 / ... opcodes applied to the argument before it is stored, in order */
    std::vector<uint8_t> casts;
};

/** Summary of calling the function at `address` on a tracked object. Results are cached in `program`. */
const box_summary& summarize_box_call(program_ir &program, Int address);

#endif //SHARP_OPT_BOX_CONSTRUCTORS_H
