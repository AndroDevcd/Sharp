//
// Created by BNunnally on 9/28/2021.
//

#ifndef SHARP_BASE_EXPRESSION_H
#define SHARP_BASE_EXPRESSION_H

#include "../../../../frontend/parser/Ast.h"

struct expression;
struct sharp_class;

void compile_base_expression(expression *e, Ast *ast);
sharp_class* compile_base_class(Ast *ast);

#endif //SHARP_BASE_EXPRESSION_H
