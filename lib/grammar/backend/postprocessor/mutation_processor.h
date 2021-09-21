//
// Created by BNunnally on 9/19/2021.
//

#ifndef SHARP_MUTATION_PROCESSOR_H
#define SHARP_MUTATION_PROCESSOR_H

#include "../../frontend/parser/Ast.h"

struct sharp_class;

void process_mutation(Ast*);
void process_mutation(sharp_class*, Ast*);
void process_generic_clone_mutations(sharp_class *with_class, Ast *ast);
void process_generic_mutations(sharp_class *with_class, sharp_class *blueprint);

#endif //SHARP_MUTATION_PROCESSOR_H
