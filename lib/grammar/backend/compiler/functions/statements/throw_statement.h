//
// Created by bknun on 6/19/2022.
//

#ifndef SHARP_THROW_STATEMENT_H
#define SHARP_THROW_STATEMENT_H

class Ast;
struct operation_schema;

void compile_throw_statement(Ast *ast, operation_schema *scheme, bool *controlPaths);

#endif //SHARP_THROW_STATEMENT_H
