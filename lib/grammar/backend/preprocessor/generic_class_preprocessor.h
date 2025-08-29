//
// Created by BNunnally on 9/8/2021.
//

#ifndef SHARP_GENERIC_CLASS_PREPROCESSOR_H
#define SHARP_GENERIC_CLASS_PREPROCESSOR_H

#include "../types/generic_type_identifier.h"

struct sharp_class;
struct Ast;

void pre_process_generic_class(sharp_class *parentClass, Ast *ast);
void pre_process_generic_identifier_list(sharp_class *genericClass, Ast *ast);
generic_type_identifier pre_process_generic_identifier(Ast *ast);

#endif //SHARP_GENERIC_CLASS_PREPROCESSOR_H
