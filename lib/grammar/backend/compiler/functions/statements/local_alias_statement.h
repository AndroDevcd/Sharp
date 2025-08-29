//
// Created by bknun on 6/11/2022.
//

#ifndef SHARP_LOCAL_ALIAS_STATEMENT_H
#define SHARP_LOCAL_ALIAS_STATEMENT_H

class Ast;
struct operation_schema;

void compile_local_alias_statement(Ast *ast, operation_schema *scheme);


#endif //SHARP_LOCAL_ALIAS_STATEMENT_H
