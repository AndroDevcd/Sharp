//
// Created by BNunnally on 9/24/2021.
//

#ifndef SHARP_PRIMARY_EXPRESSION_H
#define SHARP_BINARY_EXPRESSION_H

#include "../../../../frontend/parser/Ast.h"

struct expression;

void compile_binary_expression(expression *e, Ast *ast);

#endif //SHARP_PRIMARY_EXPRESSION_H
