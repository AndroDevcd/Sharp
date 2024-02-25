//
// Created by BNunnally on 9/24/2021.
//

#ifndef SHARP_NOT_EXPRESSION_H
#define SHARP_NOT_EXPRESSION_H

#include "../../../../frontend/parser/Ast.h"

struct expression;

void compile_not_expression(expression *e, Ast *ast);

#endif //SHARP_NOT_EXPRESSION_H
