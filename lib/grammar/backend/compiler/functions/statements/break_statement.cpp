//
// Created by bknun on 6/25/2022.
//

#include "break_statement.h"
#include "../../../../compiler_info.h"
#include "../../../types/types.h"
#include "continue_statement.h"

void compile_break_statement(Ast *ast, operation_schema *scheme) {
    List<operation_schema*> lockSchemes;
    stored_block_info* info;
    sharp_field *returnAddressField = NULL;
    stringstream ss;
    sharp_label *breakStartLabel;
    operation_schema *subScheme = new operation_schema();
    subScheme->schemeType = scheme_break;

    if(inside_block(&current_context.blockInfo, finally_block)) {
        current_file->errors->createNewError(GENERIC, ast->line, ast->col, "control cannot leave body of finally clause");
    }


    set_internal_label_name(ss, "break_begin", uniqueId++)
    breakStartLabel = create_label(ss.str(), &current_context, ast, subScheme);
    sharp_label *finallyLabel = retrieve_next_finally_label(&current_context.blockInfo);

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
            create_retain_label_value_operation(addressValueScheme, breakStartLabel);
            create_get_value_operation(&scheme_0, addressValueScheme, false, false);
            create_push_to_stack_operation(&scheme_0);

            create_local_field_access_operation(addressVariableScheme, returnAddressField);
            create_get_value_operation(&scheme_0, addressVariableScheme, false, false);
            create_pop_value_from_stack_operation(&scheme_0);
            create_unused_expression_data_operation(&scheme_0);
        )

        create_jump_operation(subScheme, finallyLabel);
    }

    create_set_label_operation(subScheme, breakStartLabel);
    retrieve_lock_schemes(&current_context.blockInfo, lockSchemes);
    for(Int i = 0; i < lockSchemes.size(); i++) {
        operation_schema* tmp = new operation_schema();
        create_unlock_operation(tmp, lockSchemes.get(i));
        add_scheme_operation(subScheme, tmp);
    }

    info = retrieve_loop_block();
    if(info->endLabel == NULL) {
        current_file->errors->createNewError(GENERIC, ast->line, ast->col, "attempt to call `break` outside of an enclosing loop");
    }

    create_jump_operation(subScheme, info->endLabel);
    add_scheme_operation(scheme, subScheme);
    current_context.blockInfo.reachable = false;
}