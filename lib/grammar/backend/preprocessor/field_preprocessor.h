//
// Created by BNunnally on 9/7/2021.
//

#ifndef SHARP_FIELD_PREPROCESSOR_H
#define SHARP_FIELD_PREPROCESSOR_H

#include "../../frontend/parser/Ast.h"

struct sharp_class;

void pre_process_field(sharp_class *owner, Ast *ast);
void pre_process_field(sharp_class *owner, uInt flags, Ast *ast);


#endif //SHARP_FIELD_PREPROCESSOR_H
