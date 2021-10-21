//
// Created by BNunnally on 10/21/2021.
//

#ifndef SHARP_POST_AST_EXPRESSION_H
#define SHARP_POST_AST_EXPRESSION_H

#include "../../../../frontend/parser/Ast.h"

struct expression;

void compile_post_ast_expression(expression *e, Ast *ast, Int startPos = 1);

#endif //SHARP_POST_AST_EXPRESSION_H
