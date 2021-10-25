//
// Created by BNunnally on 9/22/2021.
//

#include "minus_expression.h"
#include "unary_expression.h"
#include "../primary/post_ast_expression.h"

void compile_minus_expression(expression *e, Ast *ast) {
    string op = "-";
    compile_unary_prefix_expression(e, op, ast);

    compile_post_ast_expression(e, ast);
}