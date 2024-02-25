//
// Created by BNunnally on 9/26/2021.
//

#ifndef SHARP_SELF_EXPRESSION_H
#define SHARP_SELF_EXPRESSION_H

#include "../../../../frontend/parser/Ast.h"

struct expression;
struct sharp_class;

void compile_self_expression(expression *e, Ast *ast, Int endLabel);
void compile_self_expression(
        sharp_class *primaryClass,
        sharp_class *instanceClass,
        expression *e,
        Ast *ast,
        Int endLabel);

#endif //SHARP_SELF_EXPRESSION_H
