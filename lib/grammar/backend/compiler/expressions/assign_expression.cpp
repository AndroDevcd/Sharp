//
// Created by BNunnally on 12/1/2021.
//

#include "assign_expression.h"
#include "expression.h"
#include "../../types/types.h"
#include "../../../taskdelegator/task_delegator.h"

void compile_assign_expression(expression *e, Ast *ast) {
    expression left, right;
    Token &operand = ast->getToken(0);
    compile_expression(left, ast->getSubAst(0));
    compile_expression(right, ast->getSubAst(1));

    if(operand == "=") {
        sharp_function *matchedFun = NULL;
        uInt match_result = is_explicit_type_match(left.type, right.type);

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
        } else if (get_class_type(left.type) != NULL) {

            if (match_result == no_match_found) {
                match_result =
                        is_implicit_type_match(left.type, right.type,
                            match_constructor | match_initializer, matchedFun);
            }

            if(match_result == match_normal) {
                // this can possibly be optimized out ( we dont know if this is a field
                // todo: come back and see how fields with getter are handled and remove tht code
                create_function_parameter_push_operation(&e->scheme, &left.scheme, &right.scheme);
            } else {
                List<sharp_field *> params;
                List<operation_scheme *> paramOperations;

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
            }
        }
    } else {
        // todo: finish
    }
}
