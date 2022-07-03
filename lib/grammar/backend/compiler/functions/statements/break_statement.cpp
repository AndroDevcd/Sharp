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
    operation_schema *subScheme = new operation_schema();
    subScheme->schemeType = scheme_break;

    if(inside_block(&current_context.blockInfo, finally_block)) {
        current_file->errors->createNewError(GENERIC, ast->line, ast->col, "control cannot leave body of finally clause");
    }


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