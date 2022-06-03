//
// Created by bknun on 6/2/2022.
//

#ifndef SHARP_STATEMENT_COMPILER_H
#define SHARP_STATEMENT_COMPILER_H

class Ast;

void compile_statement(Ast *ast, bool *controlPaths);

#endif //SHARP_STATEMENT_COMPILER_H
