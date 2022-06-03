//
// Created by BNunnally on 12/1/2021.
//

#include "assign_expression.h"
#include "expression.h"
#include "../../types/types.h"
#include "../../../taskdelegator/task_delegator.h"
#include "../../../settings/settings.h"

void compile_assign_expression(expression *e, Ast *ast) {
    expression left, right;
    Token &operand = ast->getToken(0);
    compile_expression(left, ast->getSubAst(0));
    compile_expression(right, ast->getSubAst(1));

    sharp_function *matchedFun = NULL;
    uInt match_result = is_explicit_type_match(left.type, right.type);

    if(operand == "=") {

        if (left.type == type_field
            && left.type.field->setter != NULL) {

            sharp_field *field = left.type.field;
            if (match_result == no_match_found) {
                match_result =
                        is_implicit_type_match(left.type, right.type,
                                               constructor_only, matchedFun);
            }

            if (match_result == no_match_found
                && (is_implicit_type_match(
                    left.type, right.type,
                    match_constructor | match_initializer,
                    matchedFun) != no_match_found)) {
                currThread->currTask->file->errors->createNewError(GENERIC, ast->line, ast->col,
                                                                   "Conflicting assignment on field `" + field->name +
                                                                   "` of type `" + type_to_str(field->type) +
                                                                   "`. Cannot assign value via setter when `operator=` function is required.");
            }

            string name = "";
            string stdModule = "std";
            if (match_result != no_match_found) {
                List<sharp_field *> params;
                List<operation_scheme *> paramOperations;

                impl_location location;
                params.add(new sharp_field(
                        name, get_primary_class(&currThread->currTask->file->context), location,
                        right.type, flag_public, normal_field,
                        ast
                ));
                paramOperations.add(new operation_scheme(right.scheme));

                compile_function_call(
                        &e->scheme, params,
                        paramOperations, field->setter,
                        check_flag(field->setter->flags, flag_static),
                        false);
            } else {
                currThread->currTask->file->errors->createNewError(INCOMPATIBLE_TYPES, ast->line, ast->col,
                         "cannot assign field `" + field->fullName + "` of type `" + type_to_str(field->type)
                         + "` to expression of type `" + type_to_str(right.type) + "`.");
            }
        } else if (left.type == type_field
            && left.type.field->getter != NULL) {

            operation_scheme scheme;
            if(left.scheme.steps.size() >= 3
                && left.scheme.steps.last()->type == operation_call_instance_function
                && left.scheme.steps.last(1)->type == operation_push_value_to_stack
                && left.scheme.steps.last(2)->type == operation_get_static_class_instance) {
                left.scheme.steps.drop(3);
                create_static_field_access_operation(&scheme, left.type.field);
            } else if(left.scheme.steps.size() >= 3
                && left.scheme.steps.last()->type == operation_call_instance_function
                && left.scheme.steps.last(1)->type == operation_push_value_to_stack
                && left.scheme.steps.last(2)->type == operation_get_primary_class_instance) {
                left.scheme.steps.drop(3);
                create_primary_instance_field_access_operation(&scheme, left.type.field);
            } else if(left.scheme.steps.size() >= 1
                && left.scheme.steps.last()->type == operation_call_instance_function
                && left.scheme.steps.last()->function == left.type.field->getter) {
                left.scheme.steps.drop(1);
                create_instance_field_access_operation(&scheme, left.type.field);
            }

            left.scheme.schemeType = scheme.schemeType;
            left.scheme.steps.appendAll(scheme.steps);
        }

        if (get_class_type(left.type) != NULL || is_object_type(left.type)) {

            if (match_result == no_match_found) {
                match_result =
                        is_implicit_type_match(left.type, right.type,
                              overload_only, matchedFun);
            }

            if(match_result == match_normal) {
                if(left.type.type != type_field && options.optimize_level  == high_performance_optimization) {
                    e->scheme.copy(right.scheme);
                    e->type.copy(right.type);
                } else {
                    create_value_assignment_operation(&e->scheme, &left.scheme, &right.scheme);
                    e->type.copy(left.type);
                }

            } else if(match_result != no_match_found){
                List<sharp_field *> params;
                List<operation_scheme *> paramOperations;
                e->scheme.copy(left.scheme);
                e->type.copy(matchedFun->returnType);

                string name = "";
                string stdModule = "std";
                impl_location location;
                params.add(new sharp_field(
                        name, get_primary_class(&currThread->currTask->file->context), location,
                        right.type, flag_public, normal_field,
                        ast
                ));
                paramOperations.add(new operation_scheme(right.scheme));

                compile_function_call(
                        &e->scheme, params,
                        paramOperations, matchedFun,
                        false,
                        false);
            } else {
                currThread->currTask->file->errors->createNewError(INCOMPATIBLE_TYPES, ast->line, ast->col,
                  "expressions are not compatible, assigning `" + type_to_str(right.type)
                  + "` to expression of type `" + type_to_str(left.type) + "`.");
            }
        } else if(is_numeric_type(left.type)) {
            if(left.type.type == type_field) {
                if (match_result == no_match_found) {
                    match_result =
                            is_implicit_type_match(left.type, right.type,
                                  exclude_all, matchedFun);
                }

                if(match_result == match_normal) {
                    create_value_assignment_operation(&e->scheme, &left.scheme, &right.scheme);
                    e->type.copy(left.type);
                } else {
                    currThread->currTask->file->errors->createNewError(INCOMPATIBLE_TYPES, ast->line, ast->col,
                               "expressions are not compatible, assigning `" + type_to_str(right.type)
                               + "` to field of type `" + type_to_str(left.type) + "`.");
                }
            } else {
                currThread->currTask->file->errors->createNewError(INCOMPATIBLE_TYPES, ast->line, ast->col,
                         "expressions are not compatible, assigning `" + type_to_str(right.type)
                         + "` to expression of type `" + type_to_str(left.type) + "`.");
            }
        }
    } else {
        if(get_class_type(left.type) != NULL) {
            List<sharp_field *> params;
            List<operation_scheme *> operations;
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
                    get_class_type(left.type), left, params, operations, operand.getValue(), ast);
            e->type.copy(left.type);
            e->scheme.copy(left.scheme);
        } else if(is_numeric_type(left.type)) {
            if(left.type == type_field) {
                if (match_result == no_match_found) {
                    match_result =
                            is_implicit_type_match(left.type, right.type,
                                                   exclude_all, matchedFun);
                }

                if(match_result == match_normal) {
                    e->type.copy(left.type);

                    ALLOCATE_REGISTER_2X(0, 1, &e->scheme,
                        if(operand == "+=") {
                            create_plus_value_assignment_operation(&e->scheme, register_0, register_1, &left.scheme, &right.scheme);
                        } else if(operand == "-=") {
                            create_sub_value_assignment_operation(&e->scheme, register_0, register_1, &left.scheme, &right.scheme);
                        } else if(operand == "*=") {
                            create_mult_value_assignment_operation(&e->scheme, register_0, register_1, &left.scheme, &right.scheme);
                        } else if(operand == "/=") {
                            create_div_value_assignment_operation(&e->scheme, register_0, register_1, &left.scheme, &right.scheme);
                        } else if(operand == "%=") {
                            create_mod_value_assignment_operation(&e->scheme, register_0, register_1, &left.scheme, &right.scheme);
                        } else if(operand == "&=") {
                            create_and_value_assignment_operation(&e->scheme, register_0, register_1, &left.scheme, &right.scheme);
                        } else if(operand == "|=") {
                            create_or_value_assignment_operation(&e->scheme, register_0, register_1, &left.scheme, &right.scheme);
                        } else if(operand == "^=") {
                            create_xor_value_assignment_operation(&e->scheme, register_0, register_1, &left.scheme, &right.scheme);
                        }
                    )
                } else {
                    currThread->currTask->file->errors->createNewError(INCOMPATIBLE_TYPES, ast->line, ast->col,
                           "expressions are not compatible, assigning `" + type_to_str(right.type)
                           + "` to expression of type `" + type_to_str(left.type) + "`.");
                }
            } else {
                currThread->currTask->file->errors->createNewError(INCOMPATIBLE_TYPES, ast->line, ast->col,
                        "expressions of type `" + type_to_str(left.type)
                        + "` is not assignable.");
            }
        } else {
            currThread->currTask->file->errors->createNewError(INCOMPATIBLE_TYPES, ast->line, ast->col,
                  "expressions of type `" + type_to_str(left.type)
                  + "` is not assignable.");
        }
    }
}
