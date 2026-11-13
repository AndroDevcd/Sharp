//
// Function values in the compiler: the library class a function value is an
// instance of. See lib/runtime/memory/function_value.h for the full picture.
//

#ifndef SHARP_COMPILER_FUNCTION_VALUE_H
#define SHARP_COMPILER_FUNCTION_VALUE_H

struct sharp_class;
struct sharp_field;
struct sharp_function;

/**
 * platform.kernel#function_value and its two fields:
 *
 *      address     the index of the function (a number slot)
 *      captured    the closure object of the function that created a lambda,
 *                  or null (an object slot)
 *
 * NULL when the library is not part of the build (the compiler then reports
 * an error where the first function value is made).
 */
sharp_class* function_value_class();
sharp_field* function_value_address_field();
sharp_field* function_value_captured_field();

/**
 * Whoever makes a function value uses the class and both fields: they must be
 * part of the program even when nothing in it names them.
 *
 *      depend_on_function_value()        the function / field being compiled
 *      depend_on_function_value(user)    a given function (the finalizer, which
 *                                        has no "being compiled" any more)
 */
void depend_on_function_value();
void depend_on_function_value(sharp_function *user);

#endif //SHARP_COMPILER_FUNCTION_VALUE_H
