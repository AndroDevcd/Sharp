//
// Created by BNunnally on 9/9/2021.
//

#ifndef SHARP_BASE_CLASS_PROCESSOR_H
#define SHARP_BASE_CLASS_PROCESSOR_H

#include "../../frontend/parser/Ast.h"

struct sharp_class;

void process_base_class();
void process_base_class(sharp_class *, sharp_class *, Ast*);
sharp_class* resolve_base_class(sharp_class *, Ast*);

#endif //SHARP_BASE_CLASS_PROCESSOR_H
