//
// Created by bknun on 6/7/2022.
//

#ifndef SHARP_LOCAL_VARIABLE_DECL_H
#define SHARP_LOCAL_VARIABLE_DECL_H

class Ast;
struct operation_schema;

void compile_local_variable_statement(Ast *ast, operation_schema *scheme);

#endif //SHARP_LOCAL_VARIABLE_DECL_H
