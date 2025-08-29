//
// Created by BNunnally on 10/21/2021.
//

#ifndef SHARP_SIZEOF_EXPRESSION_H
#define SHARP_SIZEOF_EXPRESSION_H

#include "../../../../frontend/parser/Ast.h"

struct expression;

void compile_sizeof_expression(expression *e, Ast *ast);

#endif //SHARP_SIZEOF_EXPRESSION_H
