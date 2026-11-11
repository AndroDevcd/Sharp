//
// Created by bknun on 6/25/2022.
//

#include "continue_statement.h"
#include "../../../../compiler_info.h"
#include "../../../types/types.h"
#include "../../expressions/expression.h"


stored_block_info* retrieve_loop_block() {
    // the nearest enclosing loop, whatever kind it is
    block_info *info = &current_context.blockInfo;
    if(info->type == while_block || info->type == do_while_block
       || info->type == for_each_block || info->type == for_block)
        return info;

    for(Int i = (Int)info->storedItems.size() - 1; i >= 0; i--) {
        block_type type = info->storedItems.get(i)->type;
        if(type == while_block || type == do_while_block
           || type == for_each_block || type == for_block)
            return info->storedItems.get(i);
    }

    return NULL;
}

void compile_continue_statement(Ast *ast, operation_schema *scheme) {
    List<Ast*> lockExpressions;
    stored_block_info* info;
    sharp_field *returnAddressField = NULL;
    stringstream ss;
    sharp_label *continueStartLabel;
    operation_schema *subScheme = new operation_schema();
    subScheme->schemeType = scheme_continue;

    if(inside_block(&current_context.blockInfo, finally_block)) {
        create_new_error(GENERIC, ast->line, ast->col, "control cannot leave body of finally clause");
    }


    set_internal_label_name(ss, "continue_begin", uniqueId++)
    continueStartLabel = create_label(ss.str(), &current_context, ast, subScheme);
    List<sharp_label*> finallyLabels; // every finally block this jump has to run, innermost first
    retrieve_finally_labels(&current_context.blockInfo, finallyLabels, true);

    if(!finallyLabels.empty()) {
        uInt flags = flag_public;
        set_internal_variable_name(ss, "return_address", 0)
        if ((returnAddressField = resolve_local_field(ss.str(), &current_context)) == NULL) {
            sharp_type fieldType = sharp_type(type_var);
            returnAddressField = create_local_field(current_file, &current_context, ss.str(), flags, fieldType,
                                                    normal_field, ast);
            create_dependency(returnAddressField);
        }


        // run the finally blocks one at a time: each one branches back here (through return_address)
        // when it is done, and we then move on to the next enclosing one.
        for(Int i = 0; i < finallyLabels.size(); i++) {
            sharp_label *resumeLabel = continueStartLabel;
            if(i + 1 < finallyLabels.size()) {
                set_internal_label_name(ss, "finally_resume", uniqueId++)
                resumeLabel = create_label(ss.str(), &current_context, ast, subScheme);
            }

            APPLY_TEMP_SCHEME_WITH_TYPE(0, scheme_assign_value, *subScheme,
                   operation_schema *addressValueScheme = new operation_schema(scheme_get_value);
                   operation_schema *addressVariableScheme = new operation_schema();
                   create_retain_label_value_operation(addressValueScheme, resumeLabel);
                   create_get_value_operation(&scheme_0, addressValueScheme, false, false);
                   create_push_to_stack_operation(&scheme_0);

                   create_local_field_access_operation(addressVariableScheme, returnAddressField);
                   create_get_value_operation(&scheme_0, addressVariableScheme, false, false);
                   create_pop_value_from_stack_operation(&scheme_0);
                   create_unused_data_operation(&scheme_0);

                   delete addressVariableScheme;
                   delete addressValueScheme;
            )

            create_jump_operation(subScheme, finallyLabels.get(i));
            if(resumeLabel != continueStartLabel)
                create_set_label_operation(subScheme, resumeLabel);
        }
    }

    create_set_label_operation(subScheme, continueStartLabel);
    if(!finallyLabels.empty())
        create_setup_local_field_operation(subScheme, returnAddressField); // the pending jump has been taken, clear it
    retrieve_lock_expressions(&current_context.blockInfo, lockExpressions);
    for(Int i = 0; i < lockExpressions.size(); i++) {
        operation_schema* tmp = new operation_schema();
        expression e;

        compile_expression(e, lockExpressions.at(i));
        create_unlock_operation(tmp, &e.scheme);
        add_scheme_operation(subScheme, tmp);

        delete tmp;
    }

    info = retrieve_loop_block();
    if(info->repeatLoopLabel == NULL) {
        create_new_error(GENERIC, ast->line, ast->col, "attempt to call `continue` outside of an enclosing loop");
    }

    create_jump_operation(subScheme, info->repeatLoopLabel);
    add_scheme_operation(scheme, subScheme);
    current_context.blockInfo.reachable = false;
    delete subScheme;
}
