//
// Created by BNunnally on 10/21/2021.
//

#ifndef SHARP_PAREN_EXPRESSION_H
#define SHARP_PAREN_EXPRESSION_H

#include "../../../../frontend/parser/Ast.h"

struct expression;

void compile_paren_expression(expression *e, Ast *ast);

#endif //SHARP_PAREN_EXPRESSION_H
