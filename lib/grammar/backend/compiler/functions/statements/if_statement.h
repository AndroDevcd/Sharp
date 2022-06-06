//
// Created by bknun on 6/4/2022.
//

#ifndef SHARP_IF_STATEMENT_H
#define SHARP_IF_STATEMENT_H

class Ast;
struct operation_schema;

void compile_if_statement(Ast *ast, operation_schema *scheme, bool *controlPaths);

#endif //SHARP_IF_STATEMENT_H
