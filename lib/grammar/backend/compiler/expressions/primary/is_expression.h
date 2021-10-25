//
// Created by BNunnally on 10/22/2021.
//

#ifndef SHARP_IS_EXPRESSION_H
#define SHARP_IS_EXPRESSION_H

#include "../../../../frontend/parser/Ast.h"

struct expression;

void compile_is_expression(expression *e, Ast *ast, bool compileExpression = true);

#endif //SHARP_IS_EXPRESSION_H
