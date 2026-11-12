//
// Choosing between overloads that all accept a call after an implicit
// conversion - the tie-break of resolve_function() (dependency.cpp).
//
// THE PROBLEM
//     def at(index: int)    : T        // `int` is a class: the number is wrapped in an object
//     def at(index: _int64) : T        // a plain number
//     ...
//     i : decimal = 0;
//     items.at(i);
//
// Neither overload takes a decimal exactly, and both take one after a
// conversion: the decimal can be wrapped into an `int` object, or cut down to
// an _int64. Without a rule the call is "ambiguous" and does not compile.
//
// THE RULE
// When the argument is a plain number, the overload whose parameter is also a
// plain number is preferred over one whose parameter needs an object created.
// If that leaves exactly one overload, it is the one called.
//
// Why this direction: number to number costs one machine instruction (or
// nothing), number to object allocates and runs the class's constructors.
//
// C++ comparison: C++ ranks conversions the same way - a "standard conversion"
// (double to long) beats a "user-defined conversion" (double to a class through
// its constructor), so f(long) is chosen over f(Integer) for f(2.0).
//
// WHAT IT CANNOT CHANGE
// The rule is only asked when a call would otherwise be rejected as ambiguous.
// A call that compiled before still picks the overload it picked before.
//

#ifndef SHARP_OVERLOAD_PREFERENCE_H
#define SHARP_OVERLOAD_PREFERENCE_H

#include "../../List.h"

struct sharp_function;
struct sharp_field;

/**
 * `candidates` all accept `arguments` after implicit conversions. Returns the
 * one the rule above prefers, or NULL when it does not single one out (the
 * call stays ambiguous).
 */
sharp_function* prefer_plain_number_overload(
        List<sharp_function*> &candidates,
        List<sharp_field*> &arguments);

#endif //SHARP_OVERLOAD_PREFERENCE_H
