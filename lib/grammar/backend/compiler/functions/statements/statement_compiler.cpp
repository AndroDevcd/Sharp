//
// Created by bknun on 6/2/2022.
//

#include "statement_compiler.h"
#include "../../../../compiler_info.h"
#include "../../../types/types.h"
#include "../../../../settings/settings.h"
#include "return_statement.h"
#include "if_statement.h"


void compile_statement(Ast *ast, operation_schema *scheme, bool *controlPaths) {
    if(current_context.blockInfo.line == -1 || current_context.blockInfo.line < ast->line) {
        current_context.blockInfo.line = ast->line;
        APPLY_TEMP_SCHEME(1, *current_context.functionCxt->scheme,
                          create_line_record_operation(&scheme_1, ast->line);
        )
    }

    if(!current_context.blockInfo.reachable) {
        if(ast->getType() != ast_label_decl)
            create_new_warning(GENERIC, __w_general, ast, "unreachable statement");
        current_context.blockInfo.reachable = true;
    }

    switch(ast->getType()) {
        case ast_return_stmnt:
            compile_return_statement(ast, scheme, controlPaths);
            break;
        case ast_if_statement:
            compile_if_statement(ast, scheme, controlPaths);
            break;
        default:
            stringstream err;
            err << ": unknown ast type: " << ast->getType();
            currThread->currTask->file->errors->createNewError(INTERNAL_ERROR, ast->line, ast->col, err.str());
            break;
    }
}
