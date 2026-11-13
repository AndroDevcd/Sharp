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

/** how many bits an integer type has; 64 for everything that is not a sized integer */
static int integer_bits(data_type type) {
    switch(type) {
        case type_int8:  case type_uint8:  return 8;
        case type_int16: case type_uint16: return 16;
        case type_int32: case type_uint32: return 32;
        default: return 64;
    }
}

static bool is_unsigned_integer(data_type type) {
    return type == type_uint8 || type == type_uint16 || type == type_uint32 || type == type_uint64;
}

/** `value` as the integer type `type` holds it (wrapped around, like a cast) */
static Int cut_to_type(Int value, data_type type) {
    switch(type) {
        case type_int8:   return (int8_t)value;
        case type_int16:  return (int16_t)value;
        case type_int32:  return (int32_t)value;
        case type_uint8:  return (uint8_t)value;
        case type_uint16: return (uint16_t)value;
        case type_uint32: return (uint32_t)value;
        default: return value;
    }
}

void narrow_scheme_for(operation_schema *scheme, sharp_type &from, sharp_type &target) {
    if(scheme == NULL || !is_plain_number(from) || !is_plain_number(target))
        return;

    sharp_type wanted = get_real_type(target);
    if(wanted.type == type_var || integer_bits(wanted.type) == 64)
        return;     // a decimal or a 64-bit integer holds whatever arrives

    // a literal: cut now
    if(is_literal_scheme(scheme, operation_get_integer_constant)) {
        Int value = cut_to_type(scheme->steps.last()->integer, wanted.type);
        scheme->free();
        create_get_integer_constant_operation(scheme, value);
        return;
    }
    if(is_literal_scheme(scheme, operation_get_char_constant) || is_literal_scheme(scheme, operation_get_bool_constant))
        return;     // 0..255 and 0 / 1: left as they are (a character is a byte)

    // a value whose own type already fits needs nothing
    sharp_type have = get_real_type(from);
    if(have.type != type_var && have.type <= type_uint64) {
        int haveBits = integer_bits(have.type), wantBits = integer_bits(wanted.type);
        bool haveUnsigned = is_unsigned_integer(have.type), wantUnsigned = is_unsigned_integer(wanted.type);
        if(have.type == wanted.type) return;
        if(haveUnsigned == wantUnsigned && haveBits <= wantBits) return;
        if(haveUnsigned && !wantUnsigned && haveBits < wantBits) return;
    }

    if(scheme->schemeType != scheme_none)
        wrap_scheme_as_value(*scheme);
    create_cast_operation(scheme, &wanted);
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
