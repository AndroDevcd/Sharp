//
// Created by bknun on 6/25/2022.
//

#ifndef SHARP_CONTINUE_STATEMENT_H
#define SHARP_CONTINUE_STATEMENT_H

class Ast;
struct operation_schema;
struct stored_block_info;

void compile_continue_statement(Ast *ast, operation_schema *scheme);
stored_block_info* retrieve_loop_block();

#endif //SHARP_CONTINUE_STATEMENT_H
