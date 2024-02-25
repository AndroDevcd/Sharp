//
// Created by BNunnally on 9/24/2021.
//

#ifndef SHARP_BINARY_EXPRESSION_H
#define SHARP_BINARY_EXPRESSION_H

#include "../../../../frontend/parser/Ast.h"

struct expression;

void compile_binary_expression(expression *e, Ast *ast);

void compile_binary_expression(
        expression *e,
        Ast *ast,
        expression &left,
        expression &right,
        Token &operand);

#endif //SHARP_BINARY_EXPRESSION_H
