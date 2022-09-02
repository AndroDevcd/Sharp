//
// Created by bknun on 6/2/2022.
//

#include "return_statement.h"
#include "../../../../compiler_info.h"
#include "../../expressions/expression.h"
#include "../../../types/types.h"
#include "../../compiler.h"

void compile_return_statement(Ast *ast, operation_schema *scheme, bool *controlPaths) {
    current_context.blockInfo.reachable = false;

    sharp_function *function = NULL;
    uInt match_result;
    expression returnVal;
    List<Ast*> lockExpressions;
    sharp_field *returnAddressField = NULL;
    sharp_field *tempRetDataField = NULL;
    stringstream ss;
    sharp_label *returnStartLabel;
    operation_schema *subScheme = new operation_schema();
    subScheme->schemeType = scheme_return;

    if(ast->getSubAst(ast_expression)) {
        compile_expression(returnVal, ast->getSubAst(ast_expression));
    }
    else {
        returnVal.type.type = type_nil;
    }

    set_internal_label_name(ss, "return_begin", uniqueId++)
    returnStartLabel = create_label(ss.str(), &current_context, ast, subScheme);
    sharp_label *finallyLabel = retrieve_next_finally_label(&current_context.blockInfo);
    retrieve_lock_expressions(&current_context.blockInfo, lockExpressions);

    if(finallyLabel != NULL) {
        uInt flags = flag_public;
        set_internal_variable_name(ss, "return_address", 0)
        if ((returnAddressField = resolve_local_field(ss.str(), &current_context)) == NULL) {
            sharp_type fieldType = sharp_type(type_var);
            returnAddressField = create_local_field(current_file, &current_context, ss.str(), flags, fieldType,
                                                    normal_field, ast);
            create_dependency(returnAddressField);
        }


        APPLY_TEMP_SCHEME_WITH_TYPE(0, scheme_assign_value, *subScheme,
               operation_schema *addressValueScheme = new operation_schema(scheme_get_value);
               operation_schema *addressVariableScheme = new operation_schema();
               create_retain_label_value_operation(addressValueScheme,  returnStartLabel);
               create_get_value_operation(&scheme_0, addressValueScheme, false, false);
               create_push_to_stack_operation(&scheme_0);

               create_local_field_access_operation(addressVariableScheme, returnAddressField);
               create_get_value_operation(&scheme_0, addressVariableScheme, false, false);
               create_pop_value_from_stack_operation(&scheme_0);
               create_unused_data_operation(&scheme_0);

               delete addressValueScheme;
               delete addressVariableScheme;
        )

        create_jump_operation(subScheme, finallyLabel);
    }

    create_set_label_operation(subScheme, returnStartLabel);
    if(!lockExpressions.empty()) {
        if(returnVal.type == type_nil) {
            if(returnVal.scheme.schemeType != scheme_none) {
                APPLY_TEMP_SCHEME(0, *subScheme,
                    create_get_value_operation(&scheme_0, &returnVal.scheme);
                )
            }
        } else {
            uInt flags = flag_public;
            set_internal_variable_name(ss, "return_data", uniqueId++)
            tempRetDataField = create_local_field(current_file, &current_context, ss.str(), flags, returnVal.type,
                                                  normal_field, ast);
            create_dependency(tempRetDataField);

            APPLY_TEMP_SCHEME(0, *subScheme,
                operation_schema *fieldScheme = new operation_schema();
                create_local_field_access_operation(fieldScheme, tempRetDataField);
                create_value_assignment_operation(&scheme_0, fieldScheme, &returnVal.scheme);
                create_unused_data_operation(&scheme_0);

                delete fieldScheme;
            )

            returnVal.scheme.free();
            create_local_field_access_operation(&returnVal.scheme, tempRetDataField);
        }
    }

    for(Int i = 0; i < lockExpressions.size(); i++) {
        operation_schema* tmp = new operation_schema();
        expression e;

        compile_expression(e, lockExpressions.at(i));
        create_unlock_operation(tmp, &e.scheme);
        add_scheme_operation(subScheme, tmp);

        delete tmp;
    }

    match_result = is_implicit_type_match(current_context.functionCxt->returnType, returnVal.type, constructor_only, function);
    if(match_result == match_constructor) {
        operation_schema* tmp = new operation_schema();
        compile_initialization_call(ast, function, returnVal, tmp);
        create_object_return_operation(subScheme, tmp);

        delete tmp;
    } else if(is_match_normal(match_result)) {
        if(is_numeric_type(returnVal.type) && !returnVal.type.isArray) {
            create_numeric_return_operation(subScheme, &returnVal.scheme);
        }
        else if(returnVal.type.type == type_nil) {
            if(lockExpressions.empty()) {
                if(returnVal.scheme.schemeType != scheme_none) {
                    APPLY_TEMP_SCHEME(0, *subScheme,
                         create_get_value_operation(&scheme_0, &returnVal.scheme);
                    )
                }
            }

            create_return_operation(subScheme);
        }
        else create_object_return_operation(subScheme, &returnVal.scheme);
    } else if(match_result == indirect_match_w_nullability_mismatch) {
        create_new_error(INCOMPATIBLE_TYPES, ast->line, ast->col,
                                                           " expressions are not compatible, assigning nullable type of `" +
                                                                   type_to_str(returnVal.type) + "` to non nullable type of `" + type_to_str(current_context.functionCxt->returnType) + "`.");
    } else {
        current_file->errors->createNewError(GENERIC, ast->line, ast->col, "returning `" + type_to_str(returnVal.type) + "` from a function returning `"
                      + type_to_str(current_context.functionCxt->returnType) + "`.");
    }

    if(inside_block(&current_context.blockInfo, finally_block)) {
        current_file->errors->createNewError(GENERIC, ast->line, ast->col, "control cannot leave body of finally clause");
    }

    add_scheme_operation(scheme, subScheme);
    controlPaths[MAIN_CONTROL_PATH] = true;
    delete subScheme;
}
