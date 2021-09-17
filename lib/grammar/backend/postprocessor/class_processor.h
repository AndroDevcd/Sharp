//
// Created by BNunnally on 9/16/2021.
//

#ifndef SHARP_CLASS_PROCESSOR_H
#define SHARP_CLASS_PROCESSOR_H

#include "../../frontend/parser/Ast.h"

struct sharp_class;

void post_process();
void process_class(sharp_class* parentClass, sharp_class *with_class, Ast *ast);

#endif //SHARP_CLASS_PROCESSOR_H
