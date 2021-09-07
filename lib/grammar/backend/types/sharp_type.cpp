//
// Created by BNunnally on 9/2/2021.
//

#include "sharp_type.h"
#include "sharp_class.h"
#include "sharp_field.h"
#include "sharp_function.h"
#include "../../compiler_info.h"


sharp_type get_type(sharp_type &st) {
    if(st.type == type_field) {
        return st.field->type;
    }

    return sharp_type();
}

type_match_result with_result(bool check, type_match_result result) {
    if(check) return result;
    else return no_match_found;
}

CXX11_INLINE
bool has_match_result_flag(uInt flags, type_match_result flag) {
    return ((flags >> flag) & 1U);
}

// the comparer is the one to receive the value that the comparee holds
type_match_result is_explicit_type_match(sharp_type comparer, sharp_type comparee) {
    comparer = get_type(comparer);
    comparee = get_type(comparee);

    if(
         (comparer.type == comparee.type)
         || (comparer.fun != NULL && comparee.fun != NULL)
    ) {
        if(comparer.isArray != comparee.isArray) return no_match_found;

        if(comparer.fun == NULL) {
            if(comparer.type == type_class) {
                return with_result(is_explicit_type_match(
                        comparer._class, comparee._class
                    ), match_normal);
            } else return match_normal;
        } else return with_result(function_parameters_match(comparer.fun->parameters, comparee.fun->parameters, true)
                && is_explicit_type_match(comparer.fun->returnType, comparee.fun->returnType), match_normal);
    } else return no_match_found;
}

type_match_result is_implicit_type_match(
        sharp_type comparer,
        sharp_type comparee,
        uInt excludedMatches) {
    comparer = get_type(comparer);
    comparee = get_type(comparee);

    switch(comparer.type) {
        case type_null: return with_result(comparee.type == type_null, match_normal);
        case type_nil: return with_result(comparee.type == type_nil, match_normal);
        case type_any:
        case type_untyped:
        case type_undefined: return no_match_found;
        case type_function:
        case type_function_ptr:
        case type_lambda_function: {
            if(comparee.fun != NULL) {
                return with_result(comparer.isArray == comparee.isArray && function_parameters_match(comparer.fun->parameters, comparee.fun->parameters, true)
                       && is_explicit_type_match(comparer.fun->returnType, comparee.fun->returnType), match_normal);
            } else return no_match_found;
        }

        case type_field: { // shouldn't happen
            return no_match_found;
        }

        case type_class: {
            if(comparee.type == type_class) {
                if((comparer.isArray == comparee.isArray) && (is_explicit_type_match(comparer._class, comparee._class)
                    || is_implicit_type_match(comparer._class, comparee._class)))
                    return match_normal;
            }

            uInt result = no_match_found;
            List<sharp_field*> params;
            params.add(new sharp_field("mock", NULL, impl_location(),
                    comparee, flag_none, normal_field, NULL));

            if(has_match_result_flag(excludedMatches, match_constructor)
               && (resolve_function(comparer._class->name, comparer._class,
                                    params, constructor_function,
                                    match_constructor | match_initializer | match_operator_overload,
                                    NULL, true, true) != NULL)) {
                result |= match_constructor;
            }

            if(has_match_result_flag(excludedMatches, match_initializer)
               && (resolve_function("init<>"+ comparer._class->name, comparer._class,
                                    params, initializer_function,
                                    match_constructor | match_initializer | match_operator_overload,
                                    NULL, true, true) != NULL)) {
                result |= match_initializer;
            }

            if(has_match_result_flag(excludedMatches, match_operator_overload)
               && (resolve_function("operator=", comparer._class,
                                    params, operator_function,
                                    match_constructor | match_initializer | match_operator_overload,
                                    NULL, true, true) != NULL)) {
                result |= match_operator_overload;
            }

            deleteList(params);
            return (type_match_result)result;
        }

        case type_object: {
            return with_result(comparee.type == type_class
                   || comparee.type == type_object
                   || (comparee.type > type_var && comparee.isArray)
                   || (comparee.type <= type_var && comparee.isArray && !comparer.isArray), match_normal);
        }

        case type_int8:
        case type_int16:
        case type_int32:
        case type_int64:
        case type_uint8:
        case type_uint16:
        case type_uint32:
        case type_uint64:
        case type_var: {
            return with_result(comparer.isArray == comparee.isArray && comparee.type <= type_var, match_normal);
        }
        default: return no_match_found;
    }
}
