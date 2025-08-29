//
// Created by BNunnally on 9/19/2021.
//

#ifndef SHARP_DELEGATE_PROCESSOR_H
#define SHARP_DELEGATE_PROCESSOR_H

#include "../../frontend/parser/Ast.h"

struct sharp_class;

void process_delegates();
void process_delegates(sharp_class*);
void process_class_delegates(sharp_class* parentClass, sharp_class *with_class, Ast *ast);

#endif //SHARP_DELEGATE_PROCESSOR_H
