//
// Created by BNunnally on 9/24/2021.
//

#include "binary_expression.h"
#include "../unary/not_expression.h"
#include "and_expression.h"

void compile_binary_expression(expression *e, Ast *ast) {
    if(ast->hasSubAst(ast_and_e))
        compile_and_expression(e, ast->getSubAst(ast_and_e));
}