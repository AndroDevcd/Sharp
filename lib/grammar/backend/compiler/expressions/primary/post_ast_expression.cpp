//
// Created by BNunnally on 10/21/2021.
//

#include "post_ast_expression.h"
#include "../unary/post_inc_expression.h"
#include "cast_expression.h"

void compile_post_ast_expression(expression *e, Ast *ast, Int startPos) {
    for(Int i = startPos; i < ast->getSubAstCount(); i++) {
        if(ast->getSubAst(i)->getType() == ast_post_inc_e)
            compile_post_inc_expression(e, ast->getSubAst(i), false);
        else if(ast->getSubAst(ast_cast_e))
            compile_cast_expression(e, ast->getSubAst(i), false);
    }
}
