//
// Created by BNunnally on 10/24/2021.
//

#ifndef SHARP_ARRAY_EXPRESSION_H
#define SHARP_ARRAY_EXPRESSION_H

#include "../../../../frontend/parser/Ast.h"

struct expression;

void compile_array_expression(expression *e, Ast *ast);

#endif //SHARP_ARRAY_EXPRESSION_H
