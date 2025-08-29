//
// Created by BNunnally on 12/22/2021.
//

#ifndef SHARP_AND_EXPRESSION_H
#define SHARP_AND_EXPRESSION_H

#include "../../../../frontend/parser/Ast.h"

struct expression;

void compile_and_expression(expression *e, Ast *ast);

#endif //SHARP_AND_EXPRESSION_H
