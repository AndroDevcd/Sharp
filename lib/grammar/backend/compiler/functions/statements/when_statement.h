//
// Created by bknun on 7/2/2022.
//

#ifndef SHARP_WHEN_STATEMENT_H
#define SHARP_WHEN_STATEMENT_H

class Ast;
struct operation_schema;

void compile_when_statement(Ast *ast, operation_schema *scheme, bool *controlPaths);

#endif //SHARP_WHEN_STATEMENT_H
