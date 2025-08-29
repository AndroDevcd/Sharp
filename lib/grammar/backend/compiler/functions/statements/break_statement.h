//
// Created by bknun on 6/25/2022.
//

#ifndef SHARP_BREAK_STATEMENT_H
#define SHARP_BREAK_STATEMENT_H

class Ast;
struct operation_schema;

void compile_break_statement(Ast *ast, operation_schema *scheme);

#endif //SHARP_BREAK_STATEMENT_H
