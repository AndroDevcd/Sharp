//
// Created by bknun on 7/2/2022.
//

#ifndef SHARP_TRY_CATCH_STATEMENT_H
#define SHARP_TRY_CATCH_STATEMENT_H

class Ast;
struct operation_schema;

void compile_try_catch_statement(Ast *ast, operation_schema *scheme, bool *controlPaths);

#endif //SHARP_TRY_CATCH_STATEMENT_H
