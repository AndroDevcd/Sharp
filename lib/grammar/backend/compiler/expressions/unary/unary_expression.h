//
// Created by BNunnally on 9/23/2021.
//

#ifndef SHARP_UNARY_EXPRESSION_H
#define SHARP_UNARY_EXPRESSION_H

#include "../../../../frontend/parser/Ast.h"

struct expression;

void compile_unary_prefix_expression(expression *e, string &op, Ast *ast);
void compile_unary_postfix_expression(expression *e, string &op, Ast *ast);

#endif //SHARP_UNARY_EXPRESSION_H
