//
// Created by bknun on 6/12/2022.
//

#ifndef SHARP_FOR_EACH_STATEMENT_H
#define SHARP_FOR_EACH_STATEMENT_H

class Ast;
struct operation_schema;

void compile_for_each_statement(Ast *ast, operation_schema *scheme, bool *controlPaths);

#endif //SHARP_FOR_EACH_STATEMENT_H
