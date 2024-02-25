//
// Created by BNunnally on 9/10/2021.
//

#ifndef SHARP_ALIAS_PROCESSOR_H
#define SHARP_ALIAS_PROCESSOR_H

#include "../../frontend/parser/Ast.h"

struct sharp_alias;
struct sharp_class;

void process_alias(sharp_class*, Ast*);
void process_alias(sharp_alias*);

#endif //SHARP_ALIAS_PROCESSOR_H
