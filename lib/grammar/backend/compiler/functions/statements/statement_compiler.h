//
// Created by bknun on 6/2/2022.
//

#ifndef SHARP_STATEMENT_COMPILER_H
#define SHARP_STATEMENT_COMPILER_H

class Ast;
struct operation_schema;

void compile_statement(Ast *ast, operation_schema *scheme, bool *controlPaths);

#endif //SHARP_STATEMENT_COMPILER_H
