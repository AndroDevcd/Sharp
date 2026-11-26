//
// Function values: what `fn : (int)(int)` holds at run time.
//

#ifndef SHARP_FUNCTION_VALUE_H
#define SHARP_FUNCTION_VALUE_H

#include "../stdimports.h"

struct sharp_object;

/**
 * A FUNCTION VALUE IS AN OBJECT
 *
 *      add := { a: int, b: int -> (int) return a + b; };
 *      add(1, 2);
 *
 * `add` holds an instance of the library class platform.kernel#function_value:
 *
 *      [ address  : the index of the function in the method table (a number) ]
 *      [ captured : the variables the function uses from the function that
 *                   created it, or null when it uses none                    ]
 *
 * It used to be the bare index, a number. A lambda that used a local variable
 * of its creator then had to find that variable somewhere else (a slot per
 * fiber), which broke on any other fiber and when the creator called itself.
 * Now the lambda carries the variables with it.
 *
 *      C++ comparison: std::function<int(int,int)> - a callable together with
 *      what it captured - instead of a raw `int (*)(int, int)`.
 *
 * HOW IT IS CALLED (the CALLD instruction, `calld n`)
 *
 *      stack before:   [ function value ][ argument 1 ] ... [ argument n ]   <- sp
 *
 *   the arguments move down by one, over the function value:
 *                          [ argument 1 ] ... [ argument n ]
 *   captured != null   the captured object is pushed after them. A lambda that
 *                      captures has one hidden LAST parameter to receive it:
 *                          [ argument 1 ] ... [ argument n ][ captured ]
 *   captured == null   nothing more: the callee is a plain static function.
 *
 * and then the function at `address` is called like any other static function.
 * Every lambda is a static function. Slot 0 of a frame is the instance of an
 * instance function and never anything else; the captured object is a
 * parameter, not an instance.
 */

/** find the class and its two slots; called once the classes are loaded */
void setup_function_values();

/** which slots of a function value hold its function's address and what it captured (for compiled code) */
Int function_value_address_slot();
Int function_value_captured_slot();

/** the method index a function value holds (throws on null or a bad index) */
Int function_value_address(sharp_object *functionValue);

/**
 * CALLD: call the function value that sits under `argumentCount` arguments on
 * the current fiber's stack. Used by the interpreter and by compiled code.
 */
void call_function_value(Int argumentCount);

#endif //SHARP_FUNCTION_VALUE_H
