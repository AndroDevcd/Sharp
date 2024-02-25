//
// Created by bknun on 6/11/2022.
//

#ifndef SHARP_FOR_STATEMENT_H
#define SHARP_FOR_STATEMENT_H

class Ast;
struct operation_schema;

void compile_for_statement(Ast *ast, operation_schema *scheme, bool *controlPaths);

#endif //SHARP_FOR_STATEMENT_H
