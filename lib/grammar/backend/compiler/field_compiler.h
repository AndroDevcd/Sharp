//
// Created by BNunnally on 10/8/2021.
//

#ifndef SHARP_FIELD_COMPILER_H
#define SHARP_FIELD_COMPILER_H

#include "../../frontend/parser/Ast.h"

struct sharp_class;
struct sharp_field;

void compile_field(sharp_class *with_class, Ast *ast);
void compile_field(sharp_field *field, Ast *ast);
void compile_class_fields(sharp_class *with_class);
void compile_field_injection_request(sharp_field *field, Ast *ast);
void compile_static_closure_reference(sharp_field *field);


#endif //SHARP_FIELD_COMPILER_H
