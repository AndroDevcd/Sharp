//
// Created by BNunnally on 12/22/2021.
//

#include "and_expression.h"
#include "../expression.h"

void compile_and_expression(expression *e, Ast *ast) {
    expression left, right;
    Token &operand = ast->getToken(0);
    compile_expression(left, ast->getSubAst(0));
    compile_expression(right, ast->getSubAst(1));

}
