//
// Created by BNunnally on 10/5/2021.
//

#ifndef SHARP_GET_EXPRESSION_H
#define SHARP_GET_EXPRESSION_H

#include "../../../../frontend/parser/Ast.h"

struct expression;

void compile_get_expression(expression *e, Ast *ast);

#endif //SHARP_GET_EXPRESSION_H
