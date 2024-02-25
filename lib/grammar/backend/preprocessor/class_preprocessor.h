//
// Created by BNunnally on 8/31/2021.
//

#ifndef SHARP_CLASS_PREPROCESSOR_H
#define SHARP_CLASS_PREPROCESSOR_H

#include "../../../../stdimports.h"

struct sharp_class;
class Ast;

void pre_process();
void pre_process_class(sharp_class *parentClass, sharp_class *withClass, Ast *ast);
void check_decl_conflicts(Ast*, sharp_class*, string, string);

#endif //SHARP_CLASS_PREPROCESSOR_H
