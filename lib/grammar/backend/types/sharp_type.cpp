//
// Created by BNunnally on 9/2/2021.
//

#include "sharp_type.h"
#include "sharp_class.h"
#include "sharp_field.h"
#include "sharp_function.h"
#include "../../compiler_info.h"
#include "import_group.h"


void get_real_type(sharp_type &st) {
    if(st.type == type_field) {
        st.copy(st.field->type);
    }
}

native_type str_to_native_type(string &str) {
    if(str == "_int8") return type_int8;
    if(str == "_int16") return type_int16;
    if(str == "_int32") return type_int32;
    if(str == "_int64") return type_int64;
    if(str == "_uint8") return type_uint8;
    if(str == "_uint16") return type_uint16;
    if(str == "_uint32") return type_uint32;
    if(str == "_uint64") return type_uint64;
    if(str == "var") return type_var;
    if(str == "object") return type_object;
    return type_undefined;
}

string type_to_str(sharp_type &type) {
    stringstream ss;

    if(type.type == type_int8) ss << "_int8";
    else if(type.type == type_int16) ss << "_int16";
    else if(type.type == type_int32) ss << "_int32";
    else if(type.type == type_int64) ss << "_int64";
    else if(type.type == type_uint8) ss << "_uint8";
    else if(type.type == type_uint16) ss << "_uint16";
    else if(type.type == type_uint32) ss << "_uint32";
    else if(type.type == type_uint64) ss << "_uint64";
    else if(type.type == type_var) ss << "var";
    else if(type.type == type_object) ss << "object";
    else if(type.type == type_integer) ss << type.integer;
    else if(type.type == type_decimal) ss << type.decimal;
    else if(type.type == type_untyped) ss << "unknown";
    else if(type.type == type_class) ss << type._class->fullName;
    else if(type.type == type_function_ptr) {
        ss << "*(";

        for(Int i = 0; i < type.fun->parameters.size(); i++) {
            ss << type_to_str(type.fun->parameters.get(i)->type);

            if((i + 1) < type.fun->parameters.size()) {
                ss << ", ";
            }
        }

        ss << ")(";
        ss << type_to_str(type.fun->returnType) << ")";
    }
    else if(type.type == type_module) ss << type.module->name;
    else if(type.type == type_import_group) ss << type.group->name;
    else if(type.type == type_null) ss << "null";
    else if(type.type == type_nil) ss << "nil";
    else if(type.type == type_function) ss << function_to_str(type.fun);
    else if(type.type == type_field) ss << type.field->fullName << ": " << type_to_str(type.field->type);
    else ss << "undefined";

    if(type.isArray) ss << "[]";
    if(type.nullable) ss << "?";
    return ss.str();
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
uInt is_explicit_type_match(sharp_type& comparer, sharp_type& comparee) {
    get_real_type(comparer);
    get_real_type(comparee);

    if(
         (comparer.type == comparee.type && comparer.nullable == comparee.nullable)
         || (comparer.fun != NULL && comparee.fun != NULL)
    ) {
        if(comparer.isArray != comparee.isArray) return no_match_found;

        if(comparer.fun == NULL) {
            if(comparer.type == type_class) {
                return with_result(is_explicit_type_match(
                        comparer._class, comparee._class
                    ), match_normal);
            } else return match_normal;
        } else return with_result(
                function_parameters_match(comparer.fun->parameters, comparee.fun->parameters, true),
                match_normal);
    } else if (comparee.type == type_null) {
        if(!comparer.nullable) return no_match_found;

        return with_result(comparer.type == type_class
                           || comparer.type == type_object
                           || (comparer.type > type_var)
                           || (comparer.type <= type_var && comparer.isArray), match_normal);

    } else if(comparee.type == type_get_component_request) {
        type_definition *subComponent = get_type_definition(
                componentManager, comparer, *comparee.componentRequest);

        if(subComponent != NULL
            && is_explicit_type_match(comparer, *subComponent->type) != no_match_found) {
                comparee.componentRequest->resolvedTypeDefinition = subComponent;
                return match_normal;
        } else return no_match_found;
    } else return no_match_found;
}

uInt is_implicit_type_match(
        sharp_type& comparer,
        sharp_type& comparee,
        uInt excludedMatches) {
    sharp_function *ignoredMatchFun = NULL;
    return is_implicit_type_match(comparer, comparee, excludedMatches, ignoredMatchFun);
}

uInt is_implicit_type_match(
        sharp_type& comparer,
        sharp_type& comparee,
        uInt excludedMatches,
        sharp_function *&matchedFun) {
    get_real_type(comparer);
    get_real_type(comparee);

    if(comparee.type == type_get_component_request) {
        type_definition *subComponent = get_type_definition(
                componentManager, comparer, *comparee.componentRequest);

        if(subComponent != NULL
           && is_explicit_type_match(comparer, *subComponent->type) != no_match_found) {
            comparee.componentRequest->resolvedTypeDefinition = subComponent;
            return match_normal;
        } else return no_match_found;
    }

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
                if((comparer.isArray == comparee.isArray && comparer.nullable == comparee.nullable)
                    && (is_implicit_type_match(comparer._class, comparee._class)))
                    return match_normal;
            }

            uInt result = no_match_found;
            List<sharp_field*> params;
            string name = "mock";
            impl_location location;
            params.add(new sharp_field(name, NULL, location,
                    comparee, flag_none, normal_field, NULL));

            if(has_match_result_flag(excludedMatches, match_constructor)
               && ((matchedFun = resolve_function(comparer._class->name, comparer._class,
                                    params, constructor_function,
                                    match_constructor | match_initializer | match_operator_overload,
                                    NULL, true, true)) != NULL)) {
                result |= match_constructor;
            }

            if(has_match_result_flag(excludedMatches, match_initializer)
               && ((matchedFun = resolve_function("init<"+ comparer._class->name + ">", comparer._class,
                                    params, initializer_function,
                                    match_constructor | match_initializer | match_operator_overload,
                                    NULL, true, true)) != NULL)) {
                result |= match_initializer;
            }

            if(has_match_result_flag(excludedMatches, match_operator_overload)
               && ((matchedFun = resolve_function("operator=", comparer._class,
                                    params, operator_function,
                                    match_constructor | match_initializer | match_operator_overload,
                                    NULL, true, true)) != NULL)) {
                result |= match_operator_overload;
            }

            deleteList(params);
            return result;
        }

        case type_object: {
            if(comparer.nullable != comparee.nullable) return no_match_found;

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
            if(comparer.nullable != comparee.nullable) return no_match_found;
            return with_result(comparer.isArray == comparee.isArray && comparee.type <= type_var, match_normal);
        }
        default: return no_match_found;
    }
}
