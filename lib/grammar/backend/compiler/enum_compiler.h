//
// Created by bknun on 7/3/2022.
//

#ifndef SHARP_ENUM_COMPILER_H
#define SHARP_ENUM_COMPILER_H

#include "../../frontend/parser/Ast.h"

struct sharp_class;

void compile_enum_fields(sharp_class *with_class, Ast *ast);
void compile_enum_field(sharp_class *with_class, Ast *ast);

#endif //SHARP_ENUM_COMPILER_H
