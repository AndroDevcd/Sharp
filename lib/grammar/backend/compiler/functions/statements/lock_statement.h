//
// Created by bknun on 6/25/2022.
//

#ifndef SHARP_LOCK_STATEMENT_H
#define SHARP_LOCK_STATEMENT_H

class Ast;
struct operation_schema;

void compile_lock_statement(Ast *ast, operation_schema *scheme);

#endif //SHARP_LOCK_STATEMENT_H
