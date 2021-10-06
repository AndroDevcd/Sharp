//
// Created by BNunnally on 9/23/2021.
//

#include "unary_expression.h"
#include "../expression.h"
#include "../../../../taskdelegator/task_delegator.h"
#include "../../../types/types.h"
#include "../../../../settings/settings.h"

void compile_unary_prefix_expression(expression *e, string &op, Ast *ast) {
    compile_expression(*e, ast->getSubAst(ast_expression));

    if(e->type == type_undefined)
        return;

    if(e->type.isArray) {
        currThread->currTask->file->errors->createNewError(GENERIC, ast,
                                                           "expression of type `" + type_to_str(e->type) + "` must be of type `class` or numeric to use `" + op + "` operator");
    } else {
        switch(e->type.type) {
            case type_nil: {
                currThread->currTask->file->errors->createNewError(
                        GENERIC, ast, "cannot use `" + op + "` operator on expression that returns nil");
                break;
            }

            case type_untyped: {
                currThread->currTask->file->errors->createNewError(
                        INTERNAL_ERROR, ast, "cannot apply `" + op + "` operator to untyped expression.");
                break;
            }

            case type_undefined:
                break;

            case type_object: {
                currThread->currTask->file->errors->createNewError(GENERIC, ast, "expressions of type object must be casted before using `" + op + "` operator, try `" + op + "(<your-expression> as Type)` instead");
                break;
            }

            case type_integer: {
                Int value = 0;

                if(op == "-") {
                    value = -e->type.integer;
                } else if(op == "++") {
                    value = ++e->type.integer;
                } else if(op == "--") {
                    value = --e->type.integer;
                } else if(op == "!") {
                    value = !e->type.integer;

                    stringstream ss;
                    ss << "expression could be simplified to `" << (value ? "true" : "false")
                        << "`.";
                    create_new_warning(GENERIC, __w_general, ast->line, ast->col, ss.str());
                }


                create_get_integer_constant_operation(&e->scheme, value);
                break;
            }

            case type_char: {
                Int value = 0;

                if(op == "-") {
                    value = -e->type._char;
                } else if(op == "++") {
                    value = ++e->type._char;
                } else if(op == "--") {
                    value = --e->type._char;
                } else if(op == "!") {
                    value = !e->type._char;

                    stringstream ss;
                    ss << "expression could be simplified to `" << (value ? "true" : "false")
                       << "`.";
                    create_new_warning(GENERIC, __w_general, ast->line, ast->col, ss.str());
                }


                create_get_integer_constant_operation(&e->scheme, value);
                break;
            }

            case type_bool: {
                bool value = false;

                if(op == "-") {
                    value = -e->type._bool;
                } else if(op == "++" || op == "--") {
                    currThread->currTask->file->errors->createNewError(
                            GENERIC, ast, "unqualified use  of operator `" + op + "` on bool expression");
                    value = false;
                } else if(op == "!") {
                    value = !e->type._bool;

                    stringstream ss;
                    ss << "expression could be simplified to `" << (value ? "true" : "false")
                       << "`.";
                    create_new_warning(GENERIC, __w_general, ast->line, ast->col, ss.str());
                }

                create_get_bool_constant_operation(&e->scheme, value);
                break;
            }

            case type_string: {
                if(op == "-" || op == "++" || op == "--"
                    || op == "!") {
                    currThread->currTask->file->errors->createNewError(
                            GENERIC, ast, "unqualified use  of operator `" + op + "` on string expression");
                }
                break;
            }

            case type_decimal: {
                double value = 0;

                if(op == "-") {
                    value = -e->type.decimal;
                } else if(op == "++") {
                    value = ++e->type.decimal;
                } else if(op == "--") {
                    value = --e->type.decimal;
                } else if(op == "!") {
                    value = !e->type.decimal;

                    stringstream ss;
                    ss << "expression could be simplified to `" << (value == 0 ? "true" : "false")
                       << "`.";
                    create_new_warning(GENERIC, __w_general, ast->line, ast->col, ss.str());
                }

                create_get_decimal_constant_operation(&e->scheme, value);
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
            case type_var: {

                if(op == "-")
                    create_negate_operation(&e->scheme);
                else if(op == "++")
                    create_increment_operation(&e->scheme, e->type.type);
                else if(op == "--")
                    create_decrement_operation(&e->scheme, e->type.type);
                else if(op == "!")
                    create_not_operation(&e->scheme);
                break;
            }

            case type_field: {
                sharp_field *field = e->type.field;

                if((field->type.type >= type_int8 && field->type.type <= type_uint64)
                   || field->type.type == type_var) {
                    if(op == "-")
                        create_negate_operation(&e->scheme);
                    else if(op == "++")
                        create_increment_operation(&e->scheme, e->type.type);
                    else if(op == "--")
                        create_decrement_operation(&e->scheme, e->type.type);
                    else if(op == "!")
                        create_not_operation(&e->scheme);

                    e->type.type = field->type.type;
                } else if(field->type.type == type_class) {
                    List<sharp_field*> emptyParams;
                    List<operation_scheme*> noOperations;

                    compile_class_function_overload(
                            field->type._class, *e, emptyParams, noOperations, op, ast);
                } else {
                    currThread->currTask->file->errors->createNewError(
                            GENERIC, ast, "unqualified use  of operator `" + op + "` with field of type `" + type_to_str(field->type) + "`");
                }
                break;
            }

            default: {
                currThread->currTask->file->errors->createNewError(
                        GENERIC, ast, "unqualified use  of operator `" + op + "` with type `" + type_to_str(e->type) + "`");
                break;
            }
        }
    }
}