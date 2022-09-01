//
// Created by bknun on 8/29/2022.
//

#ifndef SHARP_ASM_STATEMENT_H
#define SHARP_ASM_STATEMENT_H

class Ast;
struct operation_schema;

void compile_asm_statement(Ast *ast, operation_schema *scheme);

#endif //SHARP_ASM_STATEMENT_H
