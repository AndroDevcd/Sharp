//
// Created by BNunnally on 9/2/2021.
//

#include "types.h"
#include "../../compiler_info.h"
#include "import_group.h"
#include "sharp_type.h"


sharp_type get_real_type(sharp_type &st) {
    sharp_type tmp;
    if(st.type == type_field) {
        return tmp.copy(st.field->type);
    } else if(st.type == type_string) {
        return sharp_type(type_int8, false, true);
    } else if(st.type == type_integer) {
        return sharp_type(type_int64);
    } else if(st.type == type_char) {
        return sharp_type(type_int8);
    } else if(st.type == type_bool) {
        return sharp_type(type_int8);
    } else if(st.type == type_decimal) {
        return sharp_type(type_var);
    } else return tmp.copy(st);
}

bool is_numeric_type(sharp_type &st) {
    sharp_type type = get_real_type(st);
    return type.type >= type_int8 && type.type <= type_var;
}

bool is_evaluable_type(sharp_type &st) {
    sharp_type type = get_real_type(st);
    return (is_numeric_type(type) && !type.isArray && type.type != type_function_ptr);
}

bool is_object_type(sharp_type &st) {
    sharp_type type = get_real_type(st);
    return type.type == type_object;
}

sharp_class* get_class_type(sharp_type &st) {
    if(st == type_class)
        return st._class;
    else if(st == type_field && st.field->type == type_class)
        return st.field->type._class;
    else return NULL;
}

data_type str_to_native_type(string &str) {
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
    else if(type.type == type_any) ss << "any";
    else if(type.type == type_object) ss << "object";
    else if(type.type == type_string) {
        ss << "string_literal: \"" << type._string << "\"";
        return ss.str();
    }
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
    else if(type.type == type_null) return "null";
    else if(type.type == type_nil) ss << "nil";
    else if(type.type == type_function) ss << function_to_str(type.fun);
    else if(type.type == type_lambda_function) ss << function_to_str(type.fun);
    else if(type.type == type_label) ss << "label(" << type.label->name << ")";
    else if(type.type == type_field) ss << type.field->fullName << ": " << type_to_str(type.field->type);
    else if(type.type == type_get_component_request) {
        get_component_request *request = type.componentRequest;

        if(request->resolvedTypeDefinition != NULL) {
            ss << type_to_str(*request->resolvedTypeDefinition->type);
        } else {
            if (request->componentName.empty() && request->typeDefinitionName.empty()) {
                ss << "get(?)";
            } else {
                if (!request->typeDefinitionName.empty()) {
                    if (!request->componentName.empty()) {
                        ss << "get(typeName: " << request->typeDefinitionName
                            << ", component: " << request->componentName << ")";
                    } else {
                        ss << "get(typeName: " << request->typeDefinitionName << ")";
                    }
                } else {
                    ss << "get(component: " << request->componentName << ")";
                }
            }
        }
    }
    else ss << "undefined";

    if(type.isArray) ss << "[]";
    if(type.nullable) ss << "?";
    return ss.str();
}

type_match_result nullability_check(sharp_type& comparer, sharp_type& comparee, type_match_result success_result) {
    if(is_match_normal(success_result)) {
        if(comparer.nullable) return success_result;
        else {
            if(comparee.nullable == comparer.nullable)
                return success_result;
            else return indirect_match_w_nullability_mismatch;
        }
    } else return success_result;
}

uInt is_type_definition_match(sharp_type &comparer, sharp_type &comparee) {
    if(comparee.componentRequest->resolvedTypeDefinition != NULL) {
        if(is_explicit_type_match(comparer, *comparee.componentRequest->resolvedTypeDefinition->type)
           != no_match_found) {
            return direct_match;
        }
    }

    type_definition *subComponent = get_type_definition(
            componentManager, comparer, *comparee.componentRequest);

    if(subComponent != NULL
       && is_explicit_type_match(comparer, *subComponent->type) != no_match_found) {
        comparee.componentRequest->resolvedTypeDefinition = subComponent;
        return direct_match;
    } else return no_match_found;
}

type_match_result with_result(bool check, type_match_result result) {
    if(check) return result;
    else return no_match_found;
}

CXX11_INLINE
bool has_match_result_flag(uInt flags, type_match_result flag) {
    return ((flags) & flag) == flag;
}

// the comparer is the one to receive the value that the comparee holds
uInt is_explicit_type_match(sharp_type& left, sharp_type& right) {
    auto comparer = get_real_type(left);
    auto comparee = get_real_type(right);

    if(comparee.type == type_get_component_request) {
        return is_type_definition_match(comparer, right);
    }

    if(
         (comparer.type == comparee.type &&
            (comparer.nullable == comparee.nullable || comparer.nullable))
         || (comparer.fun != NULL && comparee.fun != NULL)
         || (comparer.type != type_untyped && comparer.type != type_undefined && comparee.type == type_any) // this is a half truth and only used to fully qualify lambdas
    ) {
        if(comparer.isArray != comparee.isArray) return no_match_found;
        else if(comparer.type != type_untyped && comparer.type != type_undefined && comparee.type == type_any)
            return direct_match;

        if(comparer.fun == NULL) {
            if(comparer.type == type_class) {
                return with_result(is_explicit_type_match(
                        comparer._class, comparee._class
                    ), direct_match);
            } else return direct_match;
        } else return with_result(
                function_parameters_match(comparer.fun->parameters, comparee.fun->parameters, true),
                direct_match);
    } else if (comparee.type == type_null) {
        if(!comparer.nullable) return no_match_found;

        return with_result(comparer.type == type_class
                           || comparer.type == type_object
                           || (comparer.type > type_var)
                           || (comparer.type <= type_var && comparer.isArray), direct_match);

    } else if(comparee.type == type_get_component_request) {
        return is_type_definition_match(comparer, right);
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
        sharp_type& left,
        sharp_type& right,
        uInt excludedMatches,
        sharp_function *&matchedFun) {
    auto comparer = get_real_type(left);
    auto comparee = get_real_type(right);

    if(comparee.type == type_get_component_request) {
        return is_type_definition_match(comparer, right);
    }

    switch(comparer.type) {
        case type_null:
        case type_label:
        case type_nil: {
            return with_result(comparee.type == comparer.type
            || comparee.type == type_class
            || comparee.type == type_object
            || (comparee.type <= type_var && comparee.isArray),
            nullability_check(comparer, comparee, direct_match));
        }
        case type_any:
        case type_untyped:
        case type_undefined: return no_match_found;
        case type_function:
        case type_function_ptr:
        case type_lambda_function: {
            if(comparee.fun != NULL) {
                return with_result(comparer.isArray == comparee.isArray && function_parameters_match(comparer.fun->parameters, comparee.fun->parameters, true)
                       && is_explicit_type_match(comparer.fun->returnType, comparee.fun->returnType), nullability_check(comparer, comparee, direct_match));
            } else return no_match_found;
        }

        case type_field: { // shouldn't happen
            return no_match_found;
        }

        case type_class: {
           if(comparee.type == type_class) {
                if((comparer.isArray == comparee.isArray)
                    && (is_implicit_type_match(comparer._class, comparee._class)))
                    return nullability_check(comparer, comparee, indirect_match);
            } else if(comparee.type == type_null) {
               return nullability_check(comparer, comparee, indirect_match);
           }

            uInt result = no_match_found;
            List<sharp_field*> params;
            string name = "mock";
            impl_location location;
            params.add(new sharp_field(name, NULL, location,
                    comparee, flag_none, normal_field, NULL));

            if(!has_match_result_flag(excludedMatches, match_constructor)
               && ((matchedFun = resolve_function(comparer._class->name, comparer._class,
                                    params, constructor_function,
                                    match_constructor | match_initializer | match_operator_overload,
                                    NULL, false, true)) != NULL)) {

                result |= match_constructor;
            } else if(!has_match_result_flag(excludedMatches, match_initializer)
               && ((matchedFun = resolve_function("init<"+ comparer._class->name + ">", comparer._class,
                                    params, initializer_function,
                                    match_constructor | match_initializer | match_operator_overload,
                                    NULL, false, true)) != NULL)) {
                result |= match_initializer;
            } else if(!has_match_result_flag(excludedMatches, match_operator_overload)
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
            if(comparee.type == type_null) return true;

            return with_result(comparee.type == type_class
                   || comparee.type == type_object
                   || (comparee.type > type_var && comparee.isArray)
                   || (comparee.type <= type_var && comparee.isArray && !comparer.isArray),
                   indirect_match);
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
            if(comparee.type == type_null) return nullability_check(comparer, comparee, indirect_match);
            if(comparer.isArray || comparee.isArray) {
                if(comparee.isArray == comparer.isArray
                    && (comparee.type == comparer.type)) {
                    return nullability_check(comparer, comparee, indirect_match);
                } else
                    return no_match_found;
            } else {
                return with_result(comparee.type <= type_var || comparee.type == type_integer
                            || comparee.type == type_char || comparee.type == type_decimal
                            || comparee.type == type_bool,
                        nullability_check(comparer, comparee, indirect_match));
            }
        }
        default: return no_match_found;
    }
}

bool has_type(sharp_type &t) {
    return !(t.type == type_untyped || t.type == type_undefined);
}

sharp_type &sharp_type::copy(const sharp_type &st)  {
    free();

    _class = st._class;
    field = st.field;
    fun = st.fun;
    type = st.type;

    if(st.componentRequest)
        componentRequest = new get_component_request(*st.componentRequest);

    for(Int i = 0; i < st.unresolvedType.items.size(); i++) {
        unresolvedType.items.add(new unresolved_item(*st.unresolvedType.items.get(i)));
    }

    isArray = st.isArray;
    nullable = st.nullable;
    nullableItems = st.nullableItems;
    arrayElement = st.arrayElement;
    module = st.module;
    group = st.group;
    integer = st.integer;
    decimal = st.decimal;
    _string = st._string;
    _char = st._char;
    _bool = st._bool;
    label = st.label;

    return *this;
}
