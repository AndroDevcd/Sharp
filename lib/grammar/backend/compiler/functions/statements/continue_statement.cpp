//
// Created by bknun on 6/25/2022.
//

#include "continue_statement.h"
#include "../../../../compiler_info.h"
#include "../../../types/types.h"


stored_block_info* retrieve_loop_block() {
    stored_block_info* info;

    if((info = retrieve_block(&current_context.blockInfo, while_block)) != NULL
       || (info = retrieve_block(&current_context.blockInfo, do_while_block)) != NULL
       || (info = retrieve_block(&current_context.blockInfo, for_each_block)) != NULL
       || (info = retrieve_block(&current_context.blockInfo, for_block)) != NULL) {
        return info;
    }

    return NULL;
}

void compile_continue_statement(Ast *ast, operation_schema *scheme) {
    List<operation_schema*> lockSchemes;
    stored_block_info* info;
    operation_schema *subScheme = new operation_schema();
    subScheme->schemeType = scheme_continue;

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
    if(info->beginLabel == NULL) {
        current_file->errors->createNewError(GENERIC, ast->line, ast->col, "attempt to call `continue` outside of an enclosing loop");
    }

    create_jump_operation(subScheme, info->beginLabel);
    add_scheme_operation(scheme, subScheme);
    current_context.blockInfo.reachable = false;
}
