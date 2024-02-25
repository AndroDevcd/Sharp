//
// Created by BNunnally on 9/23/2021.
//

#ifndef SHARP_PRE_INCREMENT_EXPRESSION_H
#define SHARP_PRE_INCREMENT_EXPRESSION_H

#include "../../../../frontend/parser/Ast.h"

struct expression;

void compile_pre_increment_expression(expression *e, Ast *ast);

#endif //SHARP_PRE_INCREMENT_EXPRESSION_H
