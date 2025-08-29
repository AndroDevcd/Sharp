//
// Created by BNunnally on 10/21/2021.
//

#ifndef SHARP_POST_INC_EXPRESSION_H
#define SHARP_POST_INC_EXPRESSION_H

#include "../../../../frontend/parser/Ast.h"

struct expression;

void compile_post_inc_expression(expression *e, Ast *ast, bool compileExpression = true);

#endif //SHARP_POST_INC_EXPRESSION_H
