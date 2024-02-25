//
// Created by BNunnally on 9/16/2021.
//

#ifndef SHARP_INTERFACE_PROCESSOR_H
#define SHARP_INTERFACE_PROCESSOR_H

#include "../../frontend/parser/Ast.h"

struct sharp_class;

void process_interfaces(sharp_class*, Ast*);

#endif //SHARP_INTERFACE_PROCESSOR_H
