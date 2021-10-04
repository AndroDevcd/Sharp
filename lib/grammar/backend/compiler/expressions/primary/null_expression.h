//
// Created by BNunnally on 9/29/2021.
//

#ifndef SHARP_NULL_EXPRESSION_H
#define SHARP_NULL_EXPRESSION_H

#include "../../../../frontend/parser/Ast.h"

struct expression;

void compile_null_expression(expression *e, Ast *ast);

#endif //SHARP_NULL_EXPRESSION_H
