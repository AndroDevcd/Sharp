//
// Created by bknun on 6/19/2022.
//

#ifndef SHARP_DO_WHILE_STATEMENT_H
#define SHARP_DO_WHILE_STATEMENT_H

class Ast;
struct operation_schema;

void compile_do_while_statement(Ast *ast, operation_schema *scheme, bool *controlPaths);

#endif //SHARP_DO_WHILE_STATEMENT_H
