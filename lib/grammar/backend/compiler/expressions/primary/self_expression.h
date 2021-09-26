//
// Created by BNunnally on 9/26/2021.
//

#ifndef SHARP_SELF_EXPRESSION_H
#define SHARP_SELF_EXPRESSION_H

#include "../../../../frontend/parser/Ast.h"

struct expression;

void compile_self_expression(expression *e, Ast *ast);

#endif //SHARP_SELF_EXPRESSION_H
