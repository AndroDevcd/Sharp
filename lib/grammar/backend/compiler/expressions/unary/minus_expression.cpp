//
// Created by BNunnally on 9/22/2021.
//

#include "minus_expression.h"
#include "../expression.h"
#include "../../../../taskdelegator/task_delegator.h"

void compile_minus_expression(expression *e, Ast *ast) {
    compile_expression(*e, ast->getSubAst(ast_expression));

    if(e->type == type_undefined)
        return;

    Token *tok = ast->getToken(MINUS);
    if(e->type.isArray) {
        currThread->currTask->file->errors->createNewError(GENERIC, tok->getLine(), tok->getColumn(),
                "expression of type `" + type_to_str(e->type) + "` must be of type `class` or numeric to use `-` operator");
    } else {

    }
}