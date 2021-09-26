//
// Created by BNunnally on 9/24/2021.
//

#ifndef SHARP_LITERAL_EXPRESSION_H
#define SHARP_LITERAL_EXPRESSION_H

#include "../../../../frontend/parser/Ast.h"

struct expression;

void compile_literal_expression(expression *e, Ast *ast);
void compile_char_literal(expression *e, Token&);
void compile_integer_literal(expression *e, Token&);
void compile_hex_literal(expression *e, Token&);
void compile_string_literal(expression *e, Token&);
void compile_boolean_literal(expression *e, Token&);

#endif //SHARP_LITERAL_EXPRESSION_H
