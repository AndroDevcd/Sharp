//
// Created by BNunnally on 11/30/2021.
//

#ifndef SHARP_ELVIS_EXPRESSION_H
#define SHARP_ELVIS_EXPRESSION_H

#include "../../../frontend/parser/Ast.h"

struct expression;

void compile_elvis_expression(expression *e, Ast *ast);


#endif //SHARP_ELVIS_EXPRESSION_H
