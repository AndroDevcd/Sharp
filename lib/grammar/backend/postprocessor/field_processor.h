//
// Created by BNunnally on 9/10/2021.
//

#ifndef SHARP_FIELD_PROCESSOR_H
#define SHARP_FIELD_PROCESSOR_H

#include "../../frontend/parser/Ast.h"
#include "../compiler/expressions/expression.h"

struct sharp_field;
struct sharp_class;

void process_field(sharp_class*, Ast*);
void process_field(sharp_field*);
void process_setter(sharp_field*, Ast*);
void process_getter(sharp_field*, Ast*);
void validate_field_type(bool, sharp_field*, sharp_type&, operation_schema*, Ast*);

#endif //SHARP_FIELD_PROCESSOR_H
