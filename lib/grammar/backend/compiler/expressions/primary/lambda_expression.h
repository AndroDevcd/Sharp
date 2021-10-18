//
// Created by BNunnally on 10/17/2021.
//

#ifndef SHARP_LAMBDA_EXPRESSION_H
#define SHARP_LAMBDA_EXPRESSION_H

#include "../../../../frontend/parser/Ast.h"

struct expression;

void compile_lambda_expression(expression *e, Ast *ast);

#endif //SHARP_LAMBDA_EXPRESSION_H
