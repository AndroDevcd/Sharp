//
// Created by bknun on 4/3/2022.
//

#include "shift_expression.h"
#include "binary_expression.h"


void compile_shift_expression(expression *e, Ast *ast) {
    expression left, right;
    Token &operand = ast->getToken(0);
    compile_expression(left, ast->getSubAst(0));
    compile_expression(right, ast->getSubAst(1));

    compile_binary_expression(e, ast, left, right, operand);
}