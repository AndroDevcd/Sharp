//
// Created by BNunnally on 9/26/2021.
//

#include "dot_notation_call_expression.h"
#include "../expression.h"
#include "../../../dependency/dependancy.h"
#include "../../../../taskdelegator/task_delegator.h"

void compile_dot_notation_call_expression(expression *e, Ast *ast) {
    if(ast->hasSubAst(ast_dot_fn_e)) {
        resolve(ast, resolve_all, &e->scheme);
    } else {
        resolve(ast->getSubAst(ast_utype), resolve_all, &e->scheme);
    }

    if(e->scheme.schemeType == scheme_none) {
        currThread->currTask->file->errors->createNewError(
                GENERIC, ast, "expression of type `"
                              + type_to_str(e->type) + "` must evaluate to a value");
    }

    // todo: add post ast expression processing
}