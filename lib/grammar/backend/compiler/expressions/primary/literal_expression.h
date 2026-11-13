//
// Created by BNunnally on 9/24/2021.
//

#ifndef SHARP_LITERAL_EXPRESSION_H
#define SHARP_LITERAL_EXPRESSION_H

#include "../../../../frontend/parser/Ast.h"

struct expression;

void compile_literal_expression(expression *e, Ast *ast);

/**
 * `*e` is a string literal: make it `new string(<the literal>)`, the object an
 * assignment `s := "abc"` makes. Used where a literal has to be a string
 * object right away: "abc".size(), and a ?: with a literal on one side.
 */
void make_string_object(expression *e, Ast *ast);

/**
 * "abc".size(): what is written after a string literal (`literalExpression` is
 * the ast_literal_e node, `*e` the literal compiled by the function above).
 * Does nothing when nothing follows the literal.
 */
void compile_literal_member_access(expression *e, Ast *literalExpression);
void compile_char_literal(expression *e, Token&);
void compile_integer_literal(expression *e, Token&);
void compile_hex_literal(expression *e, Token&);
void compile_string_literal(expression *e, Token&);
void compile_boolean_literal(expression *e, Token&);
string remove_underscores(string &str);

#endif //SHARP_LITERAL_EXPRESSION_H
