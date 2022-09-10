//
// Created by BNunnally on 9/24/2021.
//

#include "binary_expression.h"
#include "../expression.h"
#include "../../../../taskdelegator/task_delegator.h"
#include "../../../types/types.h"
#include "../../../../compiler_info.h"
#include "and_expression.h"
#include "equal_expression.h"
#include "less_expression.h"
#include "shift_expression.h"
#include "add_expression.h"
#include "mult_expression.h"
#include "exponent_expression.h"
#include "../../../../settings/settings.h"

void compile_binary_expression(expression *e, Ast *ast) {
    if(ast->getType() == ast_and_e)
        compile_and_expression(e, ast);
    else if(ast->getType() == ast_equal_e)
        compile_equal_expression(e, ast);
    else if(ast->getType() == ast_less_e)
        compile_less_than_expression(e, ast);
    else if(ast->getType() == ast_shift_e)
        compile_shift_expression(e, ast);
    else if(ast->getType() == ast_add_e)
        compile_add_expression(e, ast);
    else if(ast->getType() == ast_mult_e)
        compile_mult_expression(e, ast);
    else if(ast->getType() == ast_exponent_e)
        compile_exponent_expression(e, ast);
}

void expressions_array_check(sharp_type &type, Token &op, Ast *ast) {
    if(type.isArray) {
        create_new_error(GENERIC, ast,
                                                           "expression of type `" + type_to_str(type) + "` must be of type `class` or numeric to use `" + op.getValue() + "` operator");
    }
}

/**
 * This is a quick function to process the common exceptions for an invalid right expression
 * to reduce code clutter and repetitiveness
 * @param e
 * @param left
 * @param right
 * @param operand
 * @param ast
 */
bool compile_expression_exceptions(
        expression &e,
        Token &operand,
        Ast *ast,
        bool allowObject = false) {
    switch(e.type.type) {
        case type_nil: {
            create_new_error(
                    GENERIC, ast, "cannot use `" + operand.getValue() + "` operator on expression that returns nil");
            return true;
        }

        case type_untyped: {
            create_new_error(
                    INTERNAL_ERROR, ast, "cannot apply `" + operand.getValue() + "` operator to untyped expression.");
            return true;
        }

        case type_undefined:
            return true;

        case type_object: {
            if(allowObject) return false;

            create_new_error(GENERIC, ast,
                                                               "expressions of type object must be casted before using `" + operand.getValue() + "` operator, try `(<your-expression> as Type)" + operand.getValue() + " ...` instead");
            return true;
        }

        default:
            return false;
    }
}

Int get_numeric_expression_value_as_int(expression &e) {
    switch(e.type.type) {
        case type_integer:
            return e.type.integer;

        case type_char:
            return e.type._char;

        case type_bool:
            return e.type._bool;

        case type_decimal:
            return e.type.decimal;

        default:
            return 0;
    }
}

long double get_numeric_expression_value_as_decimal(expression &e) {
    switch(e.type.type) {
        case type_integer:
            return e.type.integer;

        case type_char:
            return e.type._char;

        case type_bool:
            return e.type._bool;

        case type_decimal:
            return e.type.decimal;

        default:
            return 0;
    }
}

void  compile_binary_string_expression(
        expression *e,
        expression &left,
        expression &right,
        Token &operand,
        Ast *ast) {

    if(compile_expression_exceptions(right, operand, ast->getSubAst(1)))
        return;

    string name = "";
    sharp_type type = get_real_type(left.type);
    type.nullable = true;
    List<sharp_field*> params;
    List<operation_schema*> paramOperations;
    impl_location location(current_file, ast);

    params.add(new sharp_field(
            name, get_primary_class(&currThread->currTask->file->context), location,
            type, flag_public, normal_field, ast
    ));
    paramOperations.add(new operation_schema(left.scheme));

    string std = "std";
    sharp_class *string_class = resolve_class(get_module(std), "string", false, false);
    sharp_function *constructor = resolve_function("string", string_class, params, constructor_function, exclude_all, ast, false, false);

    if(constructor != NULL) {
        create_dependency(constructor);
        create_dependency(string_class);

        APPLY_TEMP_SCHEME(0, e->scheme,
            compile_initialization_call(ast, string_class, constructor, params, paramOperations, &scheme_0);
            scheme_0.schemeType = scheme_new_class;
        )

        deleteList(params);
        deleteList(paramOperations);
        type = get_real_type(right.type);
        params.add(new sharp_field(
                name, get_primary_class(&currThread->currTask->file->context), location,
                type, flag_public, normal_field, ast
        ));

        paramOperations.add(new operation_schema(right.scheme));
        compile_class_function_overload(string_class, *e, params, paramOperations, operand.getValue(), ast);
    } else {
        create_new_error(
                INTERNAL_ERROR, ast->getSubAst(1), "could not find support function in string class for type `" + type_to_str(left.type) + "`");
    }
}

void compile_binary_integer_expression(
        expression *e,
        expression &left,
        expression &right,
        Token &operand,
        Ast *ast) {

    expressions_array_check(left.type, operand, ast->getSubAst(0));
    expressions_array_check(right.type, operand, ast->getSubAst(1));

    if(compile_expression_exceptions(right, operand, ast->getSubAst(1)))
        return;

    switch(right.type.type) {
        case type_integer:
        case type_char:
        case type_bool:
        case type_decimal: {
            Int result = 0;
            if(operand == "&") {
                create_get_integer_constant_operation(&e->scheme,
                                                      result = (get_numeric_expression_value_as_int(left) & get_numeric_expression_value_as_int(right)));
            } else if(operand == "^") {
                create_get_integer_constant_operation(&e->scheme,
                                                      result = (get_numeric_expression_value_as_int(left) ^ get_numeric_expression_value_as_int(right)));
            } else if(operand == "|") {
                create_get_integer_constant_operation(&e->scheme,
                                                      result = (get_numeric_expression_value_as_int(left) | get_numeric_expression_value_as_int(right)));
            } else if(operand == "&&") {
                create_get_integer_constant_operation(&e->scheme,
                                                      result = (get_numeric_expression_value_as_int(left) && get_numeric_expression_value_as_int(right)));
            } else if(operand == "||") {
                create_get_integer_constant_operation(&e->scheme,
                                                      result = (get_numeric_expression_value_as_int(left) || get_numeric_expression_value_as_int(right)));
            } else if(operand == "==") {
                long double result = 0;
                create_get_decimal_constant_operation(&e->scheme,
                                                      result = (get_numeric_expression_value_as_int(left) == get_numeric_expression_value_as_int(right)));

                e->type.type = type_decimal;
                e->type.decimal = result;
                break;
            } else if(operand == "!=") {
                long double result = 0;
                create_get_decimal_constant_operation(&e->scheme,
                                                      result = (get_numeric_expression_value_as_int(left) != get_numeric_expression_value_as_int(right)));

                e->type.type = type_decimal;
                e->type.decimal = result;
                break;
            } else if(operand == "<") {
                long double result = 0;
                create_get_decimal_constant_operation(&e->scheme,
                                                      result = (get_numeric_expression_value_as_int(left) < get_numeric_expression_value_as_int(right)));

                e->type.type = type_decimal;
                e->type.decimal = result;
                break;
            } else if(operand == ">") {
                long double result = 0;
                create_get_decimal_constant_operation(&e->scheme,
                                                      result = (get_numeric_expression_value_as_int(left) > get_numeric_expression_value_as_int(right)));

                e->type.type = type_decimal;
                e->type.decimal = result;
                break;
            } else if(operand == "<=") {
                long double result = 0;
                create_get_decimal_constant_operation(&e->scheme,
                                                      result = (get_numeric_expression_value_as_int(left) <= get_numeric_expression_value_as_int(right)));

                e->type.type = type_decimal;
                e->type.decimal = result;
                break;
            } else if(operand == ">=") {
                long double result = 0;
                create_get_decimal_constant_operation(&e->scheme,
                                                      result = (get_numeric_expression_value_as_int(left) >= get_numeric_expression_value_as_int(right)));

                e->type.type = type_decimal;
                e->type.decimal = result;
                break;
            } else if(operand == "<<") {
                create_get_integer_constant_operation(&e->scheme,
                                                      result = (get_numeric_expression_value_as_int(left) << get_numeric_expression_value_as_int(right)));
            } else if(operand == ">>") {
                create_get_integer_constant_operation(&e->scheme,
                                                      result = (get_numeric_expression_value_as_int(left) >> get_numeric_expression_value_as_int(right)));
            } else if(operand == "-") {
                long double result = 0;
                create_get_decimal_constant_operation(&e->scheme,
                                                      result = (get_numeric_expression_value_as_int(left) - get_numeric_expression_value_as_int(right)));

                e->type.type = type_decimal;
                e->type.decimal = result;
                break;
            } else if(operand == "+") {
                long double result = 0;
                create_get_decimal_constant_operation(&e->scheme,
                                                      result = (get_numeric_expression_value_as_int(left) + get_numeric_expression_value_as_int(right)));

                e->type.type = type_decimal;
                e->type.decimal = result;
                break;
            } else if(operand == "/") {
                create_get_integer_constant_operation(&e->scheme,
                                                      result = (get_numeric_expression_value_as_int(left) + get_numeric_expression_value_as_int(right)));
            } else if(operand == "*") {
                long double result = 0;
                create_get_decimal_constant_operation(&e->scheme,
                                                      result = (get_numeric_expression_value_as_decimal(left) * get_numeric_expression_value_as_decimal(right)));

                e->type.type = type_decimal;
                e->type.decimal = result;
                break;
            } else if(operand == "%") {
                create_get_integer_constant_operation(&e->scheme,
                                                      result = (get_numeric_expression_value_as_int(left) % get_numeric_expression_value_as_int(right)));
            } else if(operand == "**") {
                long double result = 0;
                create_get_decimal_constant_operation(&e->scheme,
                                                      result = (pow(get_numeric_expression_value_as_int(left), get_numeric_expression_value_as_int(right))));

                e->type.type = type_decimal;
                e->type.decimal = result;
                break;
            }

            e->type.type = type_integer;
            e->type.integer = result;
            break;
        }

        case type_int8:
        case type_int16:
        case type_int32:
        case type_int64:
        case type_uint8:
        case type_uint16:
        case type_uint32:
        case type_uint64:
        case type_function_ptr:
        case type_var:
        case type_class:
        case type_field: {
            if(right.type.type == type_field || right.type.type == type_class) {
                convert_bool_expression(right, ast);
                if(!is_implicit_type_match(left.type, right.type, match_constructor)) {
                    goto error;
                }
            }

            if(operand == "||") {
                create_or_or_operation(&e->scheme, &left.scheme, &right.scheme);
            } else if(operand == "&&") {
                create_and_and_operation(&e->scheme, &left.scheme, &right.scheme);
            } else {
                ALLOCATE_REGISTER_2X(0, 1, &e->scheme,
                      create_get_value_operation(&e->scheme, &right.scheme, false);
                      create_retain_numeric_value_operation(&e->scheme, register_1);
                      create_get_integer_constant_operation(&e->scheme,
                                                            get_numeric_expression_value_as_int(
                                                                    left), false);
                      create_retain_numeric_value_operation(&e->scheme, register_0);

                      if (operand == "&") {
                          create_and_operation(&e->scheme, register_0, register_1);
                      } else if (operand == "^") {
                          create_xor_operation(&e->scheme, register_0, register_1);
                      } else if (operand == "|") {
                          create_or_operation(&e->scheme, register_0, register_1);
                      } else if (operand == "==") {
                          create_eq_eq_operation(&e->scheme, register_0, register_1);
                      } else if (operand == "!=") {
                          create_not_eq_operation(&e->scheme, register_0, register_1);
                      } else if (operand == "<") {
                          create_lt_operation(&e->scheme, register_0, register_1);
                      } else if (operand == ">") {
                          create_gt_operation(&e->scheme, register_0, register_1);
                      } else if (operand == "<=") {
                          create_lte_operation(&e->scheme, register_0, register_1);
                      } else if (operand == ">=") {
                          create_gte_operation(&e->scheme, register_0, register_1);
                      } else if (operand == "<<") {
                          create_shl_operation(&e->scheme, register_0, register_1);
                      } else if (operand == ">>") {
                          create_shr_operation(&e->scheme, register_0, register_1);
                      } else if (operand == "+") {
                          create_add_operation(&e->scheme, register_0, register_1);
                      } else if (operand == "-") {
                          create_sub_operation(&e->scheme, register_0, register_1);
                      } else if (operand == "/") {
                          create_div_operation(&e->scheme, register_0, register_1);
                      } else if (operand == "*") {
                          create_mult_operation(&e->scheme, register_0, register_1);
                      } else if (operand == "%") {
                          create_mod_operation(&e->scheme, register_0, register_1);
                      } else if (operand == "**") {
                          create_exponent_operation(&e->scheme, register_0, register_1);
                      }

                      e->scheme.schemeType = scheme_binary_math;
                )
            }

            e->type.type = type_int32;
            break;
        }

        default: {
            error:
            create_new_error(
                    GENERIC, ast->getSubAst(1), "expected numeric type, unqualified use  of operator `" + operand.getValue() + "` with type `" + type_to_str(right.type) + "`");
            break;
        }
    }
}

void compile_binary_numeric_expression(
        expression *e,
        expression &left,
        expression &right,
        Token &operand,
        Ast *ast) {

    expressions_array_check(left.type, operand, ast->getSubAst(0));
    expressions_array_check(right.type, operand, ast->getSubAst(1));

    if(compile_expression_exceptions(right, operand, ast->getSubAst(1)))
        return;

    switch(right.type.type) {
        case type_integer:
        case type_char:
        case type_bool:
        case type_decimal:
        case type_int8:
        case type_int16:
        case type_int32:
        case type_int64:
        case type_uint8:
        case type_uint16:
        case type_uint32:
        case type_uint64:
        case type_function_ptr:
        case type_var:
        case type_class:
        case type_field: {

            if(right.type.type == type_field || right.type.type == type_class) {
                convert_bool_expression(right, ast);

                if(!is_implicit_type_match(left.type, right.type, exclude_none)) {
                    goto error;
                }
            }
            if(operand == "||") {
                create_or_or_operation(&e->scheme, &left.scheme, &right.scheme);
            } else if(operand == "&&") {
                create_and_and_operation(&e->scheme, &left.scheme, &right.scheme);
            } else {
                if (left.scheme.schemeType == scheme_binary_math) {
                    ALLOCATE_REGISTER_1X(0, &e->scheme,
                         create_get_value_operation(&e->scheme, &right.scheme, false);
                         create_retain_numeric_value_operation(&e->scheme, register_0);
                         create_get_value_operation(&e->scheme, &left.scheme, false);

                         if (operand == "&") {
                             create_and_operation(&e->scheme, -1, register_0);
                         } else if (operand == "^") {
                             create_xor_operation(&e->scheme, -1, register_0);
                         } else if (operand == "|") {
                             create_or_operation(&e->scheme, -1, register_0);
                         } else if (operand == "==") {
                             create_eq_eq_operation(&e->scheme, -1, register_0);
                         } else if (operand == "!=") {
                             create_not_eq_operation(&e->scheme, -1, register_0);
                         } else if (operand == "<") {
                             create_lt_operation(&e->scheme, -1, register_0);
                         } else if (operand == ">") {
                             create_gt_operation(&e->scheme, -1, register_0);
                         } else if (operand == "<=") {
                             create_lte_operation(&e->scheme, -1, register_0);
                         } else if (operand == ">=") {
                             create_gte_operation(&e->scheme, -1, register_0);
                         } else if (operand == "<<") {
                             create_shl_operation(&e->scheme, -1, register_0);
                         } else if (operand == ">>") {
                             create_shr_operation(&e->scheme, -1, register_0);
                         } else if (operand == "+") {
                             create_add_operation(&e->scheme, -1, register_0);
                         } else if (operand == "-") {
                             create_sub_operation(&e->scheme, -1, register_0);
                         } else if (operand == "/") {
                             create_div_operation(&e->scheme, -1, register_0);
                         } else if (operand == "*") {
                             create_mult_operation(&e->scheme, -1, register_0);
                         } else if (operand == "%") {
                             create_mod_operation(&e->scheme, -1, register_0);
                         } else if (operand == "**") {
                             create_exponent_operation(&e->scheme, -1, register_0);
                         }

                         e->scheme.schemeType = scheme_accelerated_binary_math;
                    )
                } else {
                    ALLOCATE_REGISTER_2X(0, 1, &e->scheme,
                         create_get_value_operation(&e->scheme, &right.scheme, false);
                         create_retain_numeric_value_operation(&e->scheme, register_1);
                         create_get_value_operation(&e->scheme, &left.scheme, false);
                         create_retain_numeric_value_operation(&e->scheme, register_0);

                         if (operand == "&") {
                             create_and_operation(&e->scheme, register_0, register_1);
                         } else if (operand == "^") {
                             create_xor_operation(&e->scheme, register_0, register_1);
                         } else if (operand == "|") {
                             create_or_operation(&e->scheme, register_0, register_1);
                         } else if (operand == "==") {
                             create_eq_eq_operation(&e->scheme, register_0, register_1);
                         } else if (operand == "!=") {
                             create_not_eq_operation(&e->scheme, register_0, register_1);
                         } else if (operand == "<") {
                             create_lt_operation(&e->scheme, register_0, register_1);
                         } else if (operand == ">") {
                             create_gt_operation(&e->scheme, register_0, register_1);
                         } else if (operand == "<=") {
                             create_lte_operation(&e->scheme, register_0, register_1);
                         } else if (operand == ">=") {
                             create_gte_operation(&e->scheme, register_0, register_1);
                         } else if (operand == "<<") {
                             create_shl_operation(&e->scheme, register_0, register_1);
                         } else if (operand == ">>") {
                             create_shr_operation(&e->scheme, register_0, register_1);
                         } else if (operand == "+") {
                             create_add_operation(&e->scheme, register_0, register_1);
                         } else if (operand == "-") {
                             create_sub_operation(&e->scheme, register_0, register_1);
                         } else if (operand == "/") {
                             create_div_operation(&e->scheme, register_0, register_1);
                         } else if (operand == "*") {
                             create_mult_operation(&e->scheme, register_0, register_1);
                         } else if (operand == "%") {
                             create_mod_operation(&e->scheme, register_0, register_1);
                         } else if (operand == "**") {
                             create_exponent_operation(&e->scheme, register_0, register_1);
                         }

                         e->scheme.schemeType = scheme_binary_math;
                    )
                }
            }

            e->type.type = type_int32;
            break;
        }

        default: {
            error:
            create_new_error(
                    GENERIC, ast->getSubAst(1), "expected numeric type, unqualified use  of operator `" + operand.getValue() + "` with type `" + type_to_str(right.type) + "`");
            break;
        }
    }
}

void compile_binary_object_expression(
        expression *e,
        expression &left,
        expression &right,
        Token &operand,
        Ast *ast) {

    if(compile_expression_exceptions(right, operand, ast->getSubAst(1), true))
        return;

    switch(right.type.type) {
        case type_null: {
            create_get_value_operation(&e->scheme, &left.scheme, false);
            create_check_null_operation(&e->scheme);

            if(operand == "!=") {
                create_not_operation(&e->scheme);
            }

            e->scheme.schemeType = scheme_null_check;

            e->type.type = type_var;
            break;
        }

        case type_object:
        case type_class:
        case type_field: {
            if(left.type.isArray != right.type.isArray) {
                create_new_error(
                        GENERIC, ast->getSubAst(1), "mismatched array types, unqualified use  of operator `" + operand.getValue() + "` with type `" + type_to_str(right.type) + "`.");
                break;
            }

            if(right.type == type_field) {
                if(!is_implicit_type_match(left.type, right.type, exclude_none)) {
                    goto error;
                }
            }

            create_get_value_operation(&e->scheme, &right.scheme, false);
            create_push_to_stack_operation(&e->scheme);
            create_get_value_operation(&e->scheme, &left.scheme, false);
            create_push_to_stack_operation(&e->scheme);

            if(operand == "==") {
                create_instance_eq_operation(&e->scheme);
            } else if(operand == "!=") {
                create_instance_not_eq_operation(&e->scheme);
            }

            e->scheme.schemeType = scheme_instance_check;
            e->type.type = type_int8;
            break;
        }

        default: {
            error:
            create_new_error(
                    GENERIC, ast->getSubAst(1), "expected object type, unqualified use  of operator `" + operand.getValue() + "` with type `" + type_to_str(right.type) + "`");
            break;
        }
    }
}

void compile_binary_expression(
        expression *e,
        Ast *ast,
        expression &left,
        expression &right,
        Token &operand) {

    if(compile_expression_exceptions(left, operand, ast->getSubAst(0), true))
        return;

    switch(left.type.type) {

        case type_integer:
        case type_char:
        case type_bool:
        case type_decimal: {
            compile_binary_integer_expression(e, left, right, operand, ast);
            break;
        }

        case type_string: {
            if(operand == "+" || operand == "=="
                || operand == "!=" ) {
                compile_binary_string_expression(e, left, right, operand, ast);
            } else
                goto error;
            break;
        }

        case type_int8:
        case type_int16:
        case type_int32:
        case type_int64:
        case type_uint8:
        case type_uint16:
        case type_uint32:
        case type_uint64:
        case type_function_ptr:
        case type_var:
        case type_field:  {
            if(left.type == type_field) {
                if(left.type.field->type.type != type_class && left.type.field->type.type != type_object && !(left.type.field->type.type >= type_int8 && left.type.field->type.type <= type_var)) {
                    goto error;
                } else if(left.type.field->type.type == type_class) {
                    goto _class;
                } else if(left.type.field->type.type == type_object ||
                        (left.type.field->type.type >= type_int8 && left.type.field->type.type <= type_var && left.type.field->type.isArray)) {
                    goto _object;
                }
            } else if(left.type.type >= type_int8 && left.type.type <= type_var && left.type.isArray) {
                goto _object;
            }

            compile_binary_numeric_expression(e, left, right, operand, ast);
            break;
        }
        case type_object: {
            _object:
            if(operand == "==" || operand == "!=") {
                compile_binary_object_expression(e, left, right, operand, ast);
            } else
                goto error;
            break;
        }

        case type_class: {
            _class:
            if(operand == "==" || operand == "!=") {
                uInt result =
                        is_implicit_type_match(left.type, right.type, overload_only);
                if (result == no_match_found) {
                    goto error;
                } else if(result == indirect_match_w_nullability_mismatch && right.type != type_null && left.type != type_null) {
                    create_new_error(
                            NULLABILITY_MISMATCH, ast->getSubAst(0), ", unqualified use  of operator `" + operand.getValue() + "` with type `" + type_to_str(right.type) + "`");
                } else if(result == match_operator_overload) {
                    goto _overload;
                }

                compile_binary_object_expression(e, left, right, operand, ast);
            } else {
                _overload:
                expressions_array_check(left.type, operand, ast->getSubAst(0));

                List<sharp_field *> params;
                List<operation_schema *> operations;
                operations.add(new operation_schema());
                create_get_value_operation(operations.last(), &right.scheme, false);
                convert_expression_type_to_real_type(right);

                string name;
                sharp_type type;
                type.copy(right.type);
                impl_location location(current_file, ast);
                params.add(new sharp_field(name, get_primary_class(&current_file->context),
                                           location, type, flag_public, normal_field, ast));

                compile_class_function_overload(
                        get_class_type(left.type), left, params, operations, operand.getValue(), ast);
                e->type.copy(left.type);
                e->scheme.copy(left.scheme);
            }
            break;
        }

        default: {
            error:
            create_new_error(
                    GENERIC, ast->getSubAst(0), "unqualified use  of operator `"
                    + operand.getValue() + "` with type `" + type_to_str(left.type) + "` and `" + type_to_str(right.type) + "`");
            break;
        }
    }
}
