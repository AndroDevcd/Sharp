//
// See numeric_conversion.h.
//

#include "numeric_conversion.h"

/** a single number (not an array, an object, a class...) */
static bool is_plain_number(sharp_type &type) {
    sharp_type real = get_real_type(type);
    return real.type >= type_int8 && real.type <= type_var && !real.isArray;
}

/**
 * Is the scheme nothing but "load this literal"? Then its value is known here
 * and can be converted now instead of at run time.
 */
static bool is_literal_scheme(operation_schema *scheme, operation_type kind) {
    return scheme->steps.size() == 1 && scheme->steps.last()->type == kind;
}

/**
 * Append the conversion to the steps that produce the value. Those steps are
 * first made ONE "get value" step when they are more than a plain access
 * (arithmetic, a call, a ternary: schemes with a layout of their own cannot
 * simply get another step appended, see wrap_scheme_as_value in expression.h).
 */
static void append_conversion(operation_schema *scheme, numeric_conversion conversion) {
    if(scheme->steps.size() > 1 || scheme_has_own_layout(scheme->schemeType))
        wrap_scheme_as_value(*scheme);

    create_convert_number_operation(scheme, conversion);
}

static void convert_scheme(operation_schema *scheme, numeric_conversion conversion) {
    if(conversion == convert_to_decimal) {
        // 3 -> 3.0, 'a' -> 97.0, true -> 1.0
        if(is_literal_scheme(scheme, operation_get_integer_constant)) {
            Int value = scheme->steps.last()->integer;
            scheme->free();
            create_get_decimal_constant_operation(scheme, (long double)value);
            return;
        }
        if(is_literal_scheme(scheme, operation_get_char_constant)) {
            char value = scheme->steps.last()->_char;
            scheme->free();
            create_get_decimal_constant_operation(scheme, (long double)value);
            return;
        }
        if(is_literal_scheme(scheme, operation_get_bool_constant)) {
            bool value = scheme->steps.last()->_bool;
            scheme->free();
            create_get_decimal_constant_operation(scheme, value ? 1 : 0);
            return;
        }
    } else if(conversion == convert_to_integer) {
        // 2.9 -> 2 (the fraction is dropped, like D2I does at run time)
        if(is_literal_scheme(scheme, operation_get_decimal_constant)) {
            long double value = scheme->steps.last()->decimal;
            scheme->free();
            create_get_integer_constant_operation(scheme, (Int)value);
            return;
        }
    } else if(conversion == convert_to_truth) {
        if(is_literal_scheme(scheme, operation_get_decimal_constant)) {
            long double value = scheme->steps.last()->decimal;
            scheme->free();
            create_get_integer_constant_operation(scheme, value != 0 ? 1 : 0);
            return;
        }
    }

    append_conversion(scheme, conversion);
}

void convert_scheme_for(operation_schema *scheme, sharp_type &from, sharp_type &target) {
    if(scheme == NULL || !is_plain_number(from) || !is_plain_number(target))
        return;

    bool have = is_decimal_type(from), want = is_decimal_type(target);
    if(have != want)
        convert_scheme(scheme, want ? convert_to_decimal : convert_to_integer);
}

void convert_number_for(expression &e, sharp_type &target) {
    if(!is_plain_number(e.type) || !is_plain_number(target))
        return;

    bool have = is_decimal_type(e.type), want = is_decimal_type(target);
    if(have == want)
        return;

    convert_scheme(&e.scheme, want ? convert_to_decimal : convert_to_integer);

    // The expression now IS a number of the other kind. A literal stays a
    // literal (its value moves to the other member), anything else becomes a
    // plain var / _int64.
    if(want) {
        if(e.type.type == type_integer) { e.type.decimal = e.type.integer; e.type.type = type_decimal; }
        else if(e.type.type == type_char) { e.type.decimal = e.type._char; e.type.type = type_decimal; }
        else if(e.type.type == type_bool) { e.type.decimal = e.type._bool ? 1 : 0; e.type.type = type_decimal; }
        else { sharp_type decimal(type_var); e.type.copy(decimal); }
    } else {
        if(e.type.type == type_decimal) { e.type.integer = (Int)e.type.decimal; e.type.type = type_integer; }
        else { sharp_type integer(type_int64); e.type.copy(integer); }
    }
}

void convert_number_to_integer(expression &e) {
    sharp_type integer(type_int64);
    convert_number_for(e, integer);
}

void convert_number_to_condition(expression &e) {
    if(!is_plain_number(e.type) || !is_decimal_type(e.type))
        return;     // an integer is tested as it is

    convert_scheme(&e.scheme, convert_to_truth);
    sharp_type truth(type_int8);
    e.type.copy(truth);
}

bool is_decimal_operator(const string &op, bool leftIsDecimal, bool rightIsDecimal) {
    if(op == "/" || op == "**")
        return true;

    if(op == "+" || op == "-" || op == "*"
       || op == "==" || op == "!=" || op == "<" || op == ">" || op == "<=" || op == ">=")
        return leftIsDecimal || rightIsDecimal;

    return false;
}

void type_compound_assignment(operation_schema *scheme, const string &op, sharp_type &target, sharp_type &value) {
    if(scheme == NULL || scheme->steps.size() < 2)
        return;

    bool targetIsDecimal = is_decimal_type(target), valueIsDecimal = is_decimal_type(value);
    bool decimalOperation = is_decimal_operator(op, targetIsDecimal, valueIsDecimal);

    operation_step *arithmetic = scheme->steps.get(scheme->steps.size() - 2);
    arithmetic->nativeType = decimalOperation ? type_var : type_int64;

    if(decimalOperation) {
        arithmetic->leftConversion = targetIsDecimal ? convert_nothing : convert_to_decimal;
        arithmetic->rightConversion = valueIsDecimal ? convert_nothing : convert_to_decimal;
    } else {
        arithmetic->leftConversion = targetIsDecimal ? convert_to_integer : convert_nothing;
        arithmetic->rightConversion = valueIsDecimal ? convert_to_integer : convert_nothing;
    }

    // the result goes back into the target
    if(decimalOperation != targetIsDecimal)
        arithmetic->resultConversion = targetIsDecimal ? convert_to_decimal : convert_to_integer;
}
