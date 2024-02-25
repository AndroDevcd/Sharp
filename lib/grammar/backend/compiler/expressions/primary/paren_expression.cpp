//
// Created by BNunnally on 10/21/2021.
//

#include "paren_expression.h"
#include "../expression.h"
#include "post_ast_expression.h"

void compile_paren_expression(expression *e, Ast *ast) {
    compile_expression(*e, ast->getSubAst(ast_expression));

    compile_post_ast_expression(e, ast);
}