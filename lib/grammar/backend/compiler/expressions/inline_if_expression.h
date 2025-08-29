//
// Created by bknun on 7/23/2022.
//

#ifndef SHARP_INLINE_IF_EXPRESSION_H
#define SHARP_INLINE_IF_EXPRESSION_H

#include "../../../frontend/parser/Ast.h"

struct expression;

void compile_inline_if_expression(expression *e, Ast *ast);

#endif //SHARP_INLINE_IF_EXPRESSION_H
