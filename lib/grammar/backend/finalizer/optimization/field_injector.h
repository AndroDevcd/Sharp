//
// Created by bknun on 8/24/2022.
//

#ifndef SHARP_FIELD_INJECTOR_H
#define SHARP_FIELD_INJECTOR_H

#include "../../dependency/dependancy.h"

void injectAllRelevantFields();
void inject_field_initialization(sharp_class *with_class, Ast *ast, sharp_field *field);

#endif //SHARP_FIELD_INJECTOR_H
