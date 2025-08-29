//
// Created by BNunnally on 9/23/2021.
//

#include "pre_increment_expression.h"
#include "unary_expression.h"
#include "../primary/post_ast_expression.h"

void compile_pre_increment_expression(expression *e, Ast *ast) {
    string op = ast->getToken(0).getValue();
    compile_unary_prefix_expression(e, op, ast);

    compile_post_ast_expression(e, ast);
}