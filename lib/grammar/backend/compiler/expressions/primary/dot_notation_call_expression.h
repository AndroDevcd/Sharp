//
// Created by BNunnally on 9/26/2021.
//

#ifndef SHARP_DOT_NOTATION_CALL_EXPRESSION_H
#define SHARP_DOT_NOTATION_CALL_EXPRESSION_H

#include "../../../../frontend/parser/Ast.h"

struct expression;

void compile_dot_notation_call_expression(expression *e, Ast *ast);

#endif //SHARP_DOT_NOTATION_CALL_EXPRESSION_H
