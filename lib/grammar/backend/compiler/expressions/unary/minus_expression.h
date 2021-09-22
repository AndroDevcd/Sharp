//
// Created by BNunnally on 9/22/2021.
//

#ifndef SHARP_MINUS_EXPRESSION_H
#define SHARP_MINUS_EXPRESSION_H

#include "../../../../frontend/parser/Ast.h"

struct expression;

void compile_minus_expression(expression *e, Ast *ast);

#endif //SHARP_MINUS_EXPRESSION_H
