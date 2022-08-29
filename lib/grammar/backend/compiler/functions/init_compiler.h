//
// Created by bknun on 6/1/2022.
//

#ifndef SHARP_INIT_COMPILER_H
#define SHARP_INIT_COMPILER_H

class sharp_class;
class sharp_function;
class Ast;

void compile_inits();
void compile_inits(sharp_class* with_class, Ast *block);
void compile_initialization_paring(sharp_function *constructor);

#endif //SHARP_INIT_COMPILER_H
