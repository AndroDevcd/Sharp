//
// Created by bknun on 6/2/2022.
//

#include "statement_compiler.h"
#include "../../../../compiler_info.h"
#include "../../../types/types.h"
#include "../../../../settings/settings.h"
#include "return_statement.h"


void compile_statement(Ast *ast, bool *controlPaths) {
    if(curr_context.line == -1 || curr_context.line < ast->line) {
        curr_context.line = ast->line;
        APPLY_TEMP_SCHEME(1, curr_context.functionCxt->scheme->steps,
            create_line_record_operation(&scheme_1, ast->line);
        )
    }

    if(!curr_context.reachable) {
        if(ast->getType() != ast_label_decl)
            create_new_warning(GENERIC, __w_general, ast, "unreachable statement");
        curr_context.reachable = true;
    }

    switch(ast->getType()) {
        case ast_return_stmnt:
            compile_return_statement(ast, controlPaths);
            break;
        default:
            stringstream err;
            err << ": unknown ast type: " << ast->getType();
            currThread->currTask->file->errors->createNewError(INTERNAL_ERROR, ast->line, ast->col, err.str());
            break;
    }
}
