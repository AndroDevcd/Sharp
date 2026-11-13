//
// Converting a number from one KIND to the other where a value flows into a
// place that needs the other kind.
//
// A number is an integer or a decimal (is_decimal_type, types/sharp_type.h),
// and nothing at run time knows which: an integer handed to code that expects a
// decimal is simply read as garbage. Arithmetic takes care of its own operands
// (type_binary_operation in binary/binary_expression.cpp). Everywhere else a
// value moves from the expression that produced it into something with a type
// of its own, and the compiler must convert it there:
//
//      count : _int32 = 3;
//      total : var = count;            assignment        integer -> decimal
//      area(count)                     argument          for `def area(side: var)`
//      return count;                   return            from a function returning var
//      items[total]                    array index       decimal -> integer
//      if(total) { }                   condition         decimal -> 0 or 1
//
// Each of those places calls one of the functions below. A literal is converted
// at compile time (`total : var = 3` loads 3.0); anything else gets one more
// step, operation_convert_number, which becomes an I2D or D2I instruction.
//
//      C++ comparison: the implicit conversions the C++ compiler inserts for
//      `double total = count;`. Sharp's are the same idea, made explicit in the
//      bytecode.
//

#ifndef SHARP_NUMERIC_CONVERSION_H
#define SHARP_NUMERIC_CONVERSION_H

#include "expression.h"

/**
 * Make the value of `e` a number of the kind `target` holds. Does nothing when
 * either side is not a single number (an object, an array, a class) or when
 * the kinds already agree.
 *
 * Call it AFTER the types were checked against each other: it changes e.type
 * to the kind it converted to.
 */
void convert_number_for(expression &e, sharp_type &target);

/** Make the value of `e` an integer (an array index, an array size). */
void convert_number_to_integer(expression &e);

/**
 * Make the value of `e` something a conditional jump can test: an integer that
 * is 0 for false. A decimal is true when it is not 0.0 (so 0.5 is true: the
 * fraction must not be dropped by converting to an integer first).
 */
void convert_number_to_condition(expression &e);

/**
 * The same conversions for a value that only exists as a scheme (the argument
 * of a call: the generator of calls gets schemes, not expressions).
 * `from` is the type of the value, `target` the type it is given to.
 */
void convert_scheme_for(operation_schema *scheme, sharp_type &from, sharp_type &target);

/**
 * A value handed to something of a SMALLER integer type is cut to that type,
 * as a cast would do:
 *
 *      def f(x: _int8) { ... }
 *      f(200);             // x is -56, the same as (200 as _int8)
 *
 * Without this the parameter simply held 200: a number its type cannot hold.
 * Nothing is added when the value already fits by its type (an _int8 given to
 * an _int32, an _int32 given to an _int64); a literal is cut while compiling.
 *
 * `from` is the type the value has AFTER convert_scheme_for (a decimal has
 * already become an integer by then).
 *
 * C++ comparison: the implicit conversion in `int8_t x = 200;`.
 */
void narrow_scheme_for(operation_schema *scheme, sharp_type &from, sharp_type &target);

/**
 * Is `left <op> right` calculated with decimals?
 *
 *      /  **                   always
 *      + - *  == != < > <= >=  when either side is a decimal
 *      % & | ^ << >>           never (integers)
 */
bool is_decimal_operator(const string &op, bool leftIsDecimal, bool rightIsDecimal);

/**
 * Type the arithmetic step of a compound assignment `target <op>= value`
 * (the schemes made by create_plus_value_assignment_operation and friends:
 * the arithmetic is the step before the last one).
 *
 * It is `target = target <op> value`: the operation is typed like the binary
 * operator, and its result is converted back to the kind of the target.
 *
 *      count /= 2      (count: _int32)   i2d, i2d, div, d2i, store
 *      total %= 3      (total: var)      d2i, mod, i2d, store
 *
 * `op` is the operator without the `=` ("+", "/", ...).
 */
void type_compound_assignment(operation_schema *scheme, const string &op, sharp_type &target, sharp_type &value);

#endif //SHARP_NUMERIC_CONVERSION_H
