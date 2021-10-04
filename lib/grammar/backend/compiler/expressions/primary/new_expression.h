//
// Created by BNunnally on 9/29/2021.
//

#ifndef SHARP_NEW_EXPRESSION_H
#define SHARP_NEW_EXPRESSION_H

#include "../../../../frontend/parser/Ast.h"

struct expression;
struct sharp_class;
struct sharp_type;

void compile_new_expression(expression *e, Ast *ast);
void compile_new_array_expression(sharp_type *newType, expression *e, Ast *ast);

#endif //SHARP_NEW_EXPRESSION_H
