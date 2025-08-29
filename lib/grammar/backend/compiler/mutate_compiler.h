//
// Created by bknun on 7/3/2022.
//

#ifndef SHARP_MUTATE_COMPILER_H
#define SHARP_MUTATE_COMPILER_H

#include "../../frontend/parser/Ast.h"

class sharp_class;
class sharp_file;

void compile_mutation(Ast* ast);
void compile_global_mutations(sharp_file *file);

#endif //SHARP_MUTATE_COMPILER_H
