//
// Created by BNunnally on 12/22/2021.
//

#include "and_expression.h"
#include "../expression.h"
#include "../../../../taskdelegator/task_delegator.h"
#include "../../../../frontend/ErrorManager.h"
#include "../../../types/types.h"
#include "../../../../compiler_info.h"

void and_expressions_array_check(sharp_type &type, Token &op, Ast *ast) {
    if(type.isArray) {
        currThread->currTask->file->errors->createNewError(GENERIC, ast,
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
bool compile_and_expression_exceptions(
        expression &e,
        Token &operand,
        Ast *ast) {
    switch(e.type.type) {
        case type_nil: {
            currThread->currTask->file->errors->createNewError(
                    GENERIC, ast, "cannot use `" + operand.getValue() + "` operator on expression that returns nil");
            return true;
        }

        case type_untyped: {
            currThread->currTask->file->errors->createNewError(
                    INTERNAL_ERROR, ast, "cannot apply `" + operand.getValue() + "` operator to untyped expression.");
            return true;
        }

        case type_undefined:
            return true;

        case type_object: {
            currThread->currTask->file->errors->createNewError(GENERIC, ast,
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

void compile_integer_and_expression(
        expression *e,
        expression &left,
        expression &right,
        Token &operand,
        Ast *ast) {

    if(compile_and_expression_exceptions(right, operand, ast->getSubAst(1)))
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
        case type_field: {
            if(right.type == type_field) {
                if(!is_implicit_type_match(left.type, right.type, match_normal)) {
                    goto error;
                }
            }

            ALLOCATE_REGISTER_2X(0, 1, &e->scheme,
                     create_get_value_operation(&e->scheme, &right.scheme, false);
                     create_retain_numeric_value_operation(&e->scheme, register_1);
                     create_get_integer_constant_operation(&e->scheme, get_numeric_expression_value_as_int(left), false);
                     create_retain_numeric_value_operation(&e->scheme, register_0);

                     if(operand == "&") {
                         create_and_operation(&e->scheme, register_0, register_1);
                     } else if(operand == "^") {
                         create_xor_operation(&e->scheme, register_0, register_1);
                     } else if(operand == "|") {
                         create_or_operation(&e->scheme, register_0, register_1);
                     } else if(operand == "&&") {
                         create_and_and_operation(&e->scheme, register_0, register_1);
                     } else if(operand == "||") {
                         create_or_or_operation(&e->scheme, register_0, register_1);
                     }
            )

            e->type.type = type_int64;
            break;
        }

        default: {
            error:
            currThread->currTask->file->errors->createNewError(
                    GENERIC, ast->getSubAst(1), "expected numeric type, unqualified use  of operator `" + operand.getValue() + "` with type `" + type_to_str(right.type) + "`");
            break;
        }
    }
}

void compile_numeric_and_expression(
        expression *e,
        expression &left,
        expression &right,
        Token &operand,
        Ast *ast) {

    if(compile_and_expression_exceptions(right, operand, ast->getSubAst(1)))
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
        case type_field: {

            if(right.type == type_field) {
                if(!is_implicit_type_match(left.type, right.type, match_normal)) {
                    goto error;
                }
            }

            ALLOCATE_REGISTER_2X(0, 1, &e->scheme,
                    create_get_value_operation(&e->scheme, &right.scheme, false);
                    create_retain_numeric_value_operation(&e->scheme, register_1);
                    create_get_value_operation(&e->scheme, &right.scheme, false);
                    create_retain_numeric_value_operation(&e->scheme, register_0);

                    if(operand == "&") {
                        create_and_operation(&e->scheme, register_0, register_1);
                    } else if(operand == "^") {
                        create_xor_operation(&e->scheme, register_0, register_1);
                    } else if(operand == "|") {
                        create_or_operation(&e->scheme, register_0, register_1);
                    } else if(operand == "&&") {
                        create_and_and_operation(&e->scheme, register_0, register_1);
                    } else if(operand == "||") {
                        create_or_or_operation(&e->scheme, register_0, register_1);
                    }
            )

            e->type.type = type_int64;
            break;
        }

        default: {
            error:
            currThread->currTask->file->errors->createNewError(
                    GENERIC, ast->getSubAst(1), "expected numeric type, unqualified use  of operator `" + operand.getValue() + "` with type `" + type_to_str(right.type) + "`");
            break;
        }
    }
}

void compile_and_expression(expression *e, Ast *ast) {
    expression left, right;
    Token &operand = ast->getToken(0);
    compile_expression(left, ast->getSubAst(0));
    compile_expression(right, ast->getSubAst(1));

    if(compile_and_expression_exceptions(left, operand, ast->getSubAst(0)))
        return;

    and_expressions_array_check(left.type, operand, ast->getSubAst(0));
    and_expressions_array_check(right.type, operand, ast->getSubAst(1));

    switch(left.type.type) {

        case type_integer:
        case type_char:
        case type_bool:
        case type_decimal: {
            compile_integer_and_expression(e, left, right, operand, ast);
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
                if(left.type.field->type.type != type_class && !(left.type.field->type.type >= type_int8 && left.type.field->type.type <= type_var)) {
                    goto error;
                } else if(left.type.field->type.type == type_class) {
                    goto _class;
                }
            }

            compile_numeric_and_expression(e, left, right, operand, ast);
            break;
        }

        case type_class: {
            _class:
            List<sharp_field*> params;
            List<operation_scheme*> operations;
            operations.add(new operation_scheme());
            create_get_value_operation(operations.last(), &right.scheme, false);
            convert_expression_type_to_real_type(right);

            string name;
            sharp_type type;
            type.copy(right.type);
            impl_location location(current_file, ast);
            params.add(new sharp_field(name, get_primary_class(&current_file->context),
                                       location, type, flag_public, normal_field, ast));

            compile_class_function_overload(
                    get_class_type(left.type), *e, params, operations, operand.getValue(), ast);
            break;
        }

        default: {
            error:
            currThread->currTask->file->errors->createNewError(
                    GENERIC, ast->getSubAst(0), "unqualified use  of operator `" + operand.getValue() + "` with type `" + type_to_str(e->type) + "`");
            break;
        }
    }
}
