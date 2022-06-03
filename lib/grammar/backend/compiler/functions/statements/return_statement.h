//
// Created by bknun on 6/2/2022.
//

#ifndef SHARP_RETURN_STATEMENT_H
#define SHARP_RETURN_STATEMENT_H

class Ast;

void compile_return_statement(Ast *ast, bool *controlPaths);

#endif //SHARP_RETURN_STATEMENT_H
