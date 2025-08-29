//
// Created by BNunnally on 10/21/2021.
//

#ifndef SHARP_CAST_EXPRESSION_H
#define SHARP_CAST_EXPRESSION_H

#include "../../../../frontend/parser/Ast.h"

struct expression;

void compile_cast_expression(expression *e, Ast *ast);

#endif //SHARP_CAST_EXPRESSION_H
