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

    operation_schema *schema = new operation_schema();
    if(op == "-")
        schema->schemeType = scheme_negate_value;
    else if(op == "++")
        schema->schemeType = scheme_inc_value;
    else if(op == "--")
        schema->schemeType = scheme_dec_value;
    else if(op == "!")
        schema->schemeType = scheme_not_value;

    create_get_value_operation(schema, &e->scheme, false, false);

    if(e->type.isArray) {
        create_new_error(GENERIC, ast,
                                                           "expression of type `" + type_to_str(e->type) + "` must be of type `class` or numeric to use `" + op + "` operator");
    } else {
        switch(e->type.type) {
            case type_nil: {
                create_new_error(
                        GENERIC, ast, "cannot use `" + op + "` operator on expression that returns nil");
                break;
            }

            case type_untyped: {
                create_new_error(
                        INTERNAL_ERROR, ast, "cannot apply `" + op + "` operator to untyped expression.");
                break;
            }

            case type_undefined:
                break;

            case type_object: {
                create_new_error(GENERIC, ast, "expressions of type object must be casted before using `" + op + "` operator, try `" + op + "(<your-expression> as Type)` instead");
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


                schema->free();
                create_get_integer_constant_operation(schema, value, false, false);

                e->type.integer = value;
                e->scheme.free();
                e->scheme.copy(*schema);
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


                schema->free();
                create_get_integer_constant_operation(schema, value, false, false);

                e->type._char = value;
                e->scheme.free();
                e->scheme.copy(*schema);
                break;
            }

            case type_bool: {
                bool value = false;

                if(op == "-") {
                    value = !e->type._bool;
                } else if(op == "++" || op == "--") {
                    create_new_error(
                            GENERIC, ast, "unqualified use  of operator `" + op + "` on bool expression");
                    value = false;
                } else if(op == "!") {
                    value = !e->type._bool;

                    stringstream ss;
                    ss << "expression could be simplified to `" << (value ? "true" : "false")
                       << "`.";
                    create_new_warning(GENERIC, __w_general, ast->line, ast->col, ss.str());
                }

                schema->free();
                create_get_bool_constant_operation(schema, value, false, false);

                e->type._bool = value;
                e->scheme.free();
                e->scheme.copy(*schema);
                break;
            }

            case type_string: {
                if(op == "-" || op == "++" || op == "--"
                    || op == "!") {
                    create_new_error(
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

                e->type.decimal = value;
                schema->free();
                create_get_decimal_constant_operation(schema, value, false, false);

                e->scheme.free();
                e->scheme.copy(*schema);
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
                    create_negate_operation(schema);
                else if(op == "++")
                    create_increment_operation(schema, e->type.type);
                else if(op == "--")
                    create_decrement_operation(schema, e->type.type);
                else if(op == "!")
                    create_not_operation(schema);

                e->scheme.free();
                e->scheme.copy(*schema);
                break;
            }

            case type_field: {
                sharp_field *field = e->type.field;

                if((field->type.type >= type_int8 && field->type.type <= type_uint64)
                   || field->type.type == type_var) {
                    if(op == "-")
                        create_negate_operation(schema);
                    else if(op == "++")
                        create_increment_operation(schema, field->type.type);
                    else if(op == "--")
                        create_decrement_operation(schema, field->type.type);
                    else if(op == "!")
                        create_not_operation(schema);

                    e->type.type = field->type.type;

                    e->scheme.free();
                    e->scheme.copy(*schema);
                } else if(field->type.type == type_class) {
                    goto _class;
                } else {
                    create_new_error(
                            GENERIC, ast, "unqualified use  of operator `" + op + "` with field of type `" + type_to_str(field->type) + "`");
                }
                break;
            }

            case type_class: {
                _class:
                List<sharp_field*> emptyParams;
                List<operation_schema*> noOperations;
                sharp_class *sc = get_class_type(e->type);

                auto fun = compile_class_function_overload(
                        sc, *e, emptyParams, noOperations, op, ast);
                create_dependency(fun);
                break;
            }

            default: {
                create_new_error(
                        GENERIC, ast, "unqualified use  of operator `" + op + "` with type `" + type_to_str(e->type) + "`");
                break;
            }
        }

        delete schema;
    }
}


void compile_unary_postfix_expression(expression *e, string &op, Ast *ast, bool compileExpression) {
    if(compileExpression)
        compile_expression(*e, ast->getSubAst(ast_expression));

    if(e->type == type_undefined)
        return;

    if(e->type.isArray) {
        create_new_error(GENERIC, ast,
                       "expression of type `" + type_to_str(e->type) + "` must be of type `class` or numeric to use `" + op + "` operator");
    } else {
        switch(e->type.type) {
            case type_nil: {
                create_new_error(
                        GENERIC, ast, "cannot use `" + op + "` operator on expression that returns nil");
                break;
            }

            case type_untyped: {
                create_new_error(
                        INTERNAL_ERROR, ast, "cannot apply `" + op + "` operator to untyped expression.");
                break;
            }

            case type_undefined:
                break;

            case type_object: {
                create_new_error(GENERIC, ast, "expressions of type object must be casted before using `" + op + "` operator, try `(<your-expression> as Type)" + op + "` instead");
                break;
            }

            case type_integer: {
                Int value = 0;

                if(op == "++") {
                    value = e->type.integer++;
                } else if(op == "--") {
                    value = e->type.integer--;
                }


                create_get_integer_constant_operation(&e->scheme, value);
                break;
            }

            case type_char: {
                Int value = 0;

                if(op == "++") {
                    value = e->type._char++;
                } else if(op == "--") {
                    value = e->type._char--;
                }


                create_get_integer_constant_operation(&e->scheme, value);
                break;
            }

            case type_bool: {
                bool value = false;

                if(op == "++" || op == "--") {
                    create_new_error(
                            GENERIC, ast, "unqualified use  of operator `" + op + "` on bool expression");
                    value = false;
                }

                create_get_bool_constant_operation(&e->scheme, value);
                break;
            }

            case type_string: {
                if(op == "-" || op == "++" || op == "--"
                   || op == "!") {
                    create_new_error(
                            GENERIC, ast, "unqualified use  of operator `" + op + "` on string expression");
                }
                break;
            }

            case type_decimal: {
                double value = 0;

                if(op == "++") {
                    value = e->type.decimal++;
                } else if(op == "--") {
                    value = e->type.decimal--;
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
                // do nothing because ++ or -- value will never be captured
                break;
            }

            case type_field: {
                sharp_field *field = e->type.field;

                if(((field->type.type >= type_int8 && field->type.type <= type_uint64)
                   || field->type.type == type_var) && !field->type.isArray) {
                    create_post_scheme_start_operation(&e->scheme);

                    create_push_to_stack_operation(&e->scheme);

                    if(op == "++")
                        create_increment_operation(&e->scheme, field->type.type);
                    else if(op == "--")
                        create_decrement_operation(&e->scheme, field->type.type);

                    create_pop_value_from_stack_operation(&e->scheme);

                    e->scheme.schemeType = scheme_post_increment;
                    e->type.type = field->type.type;
                } else if(field->type.type == type_class) {
                    goto _class;
                } else {
                    create_new_error(
                            GENERIC, ast, "unqualified use  of operator `" + op + "` with field of type `" + type_to_str(field->type) + "`");
                }
                break;
            }

            case type_class: {
                _class:
                List<sharp_field*> params;
                List<operation_schema*> operations;
                sharp_class *sc = get_class_type(e->type);

                operations.add(new operation_schema());
                create_get_integer_constant_operation(
                        operations.last(), 1);

                string name;
                sharp_type type(type_var);
                impl_location location(current_file, ast);
                params.add(new sharp_field(name, get_primary_class(&current_file->ctx),
                                           location, type, flag_public, normal_field, ast));

                auto fun = compile_class_function_overload(
                        sc, *e, params, operations, op, ast);
                create_dependency(fun);
                break;
            }

            default: {
                create_new_error(
                        GENERIC, ast, "unqualified use  of operator `" + op + "` with type `" + type_to_str(e->type) + "`");
                break;
            }
        }
    }
}
