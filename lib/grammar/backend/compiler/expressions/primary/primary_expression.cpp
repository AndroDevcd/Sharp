//
// Created by BNunnally on 9/24/2021.
//

#include "primary_expression.h"
#include "../unary/not_expression.h"
#include "literal_expression.h"
#include "self_expression.h"

void compile_primary_expression(expression *e, Ast *ast) {
    if(ast->hasSubAst(ast_not_e))
        compile_not_expression(e, ast->getSubAst(ast_not_e));
    else if(ast->hasSubAst(ast_literal_e))
        compile_literal_expression(e, ast->getSubAst(ast_literal_e)->getSubAst(ast_literal));
    else if(ast->hasSubAst(ast_self_e))
        compile_self_expression(e, ast->getSubAst(ast_self_e));
}