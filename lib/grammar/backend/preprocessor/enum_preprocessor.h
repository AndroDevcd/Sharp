//
// Created by BNunnally on 9/7/2021.
//

#ifndef SHARP_ENUM_PREPROCESSOR_H
#define SHARP_ENUM_PREPROCESSOR_H

struct sharp_class;
struct Ast;

void pre_process_enum(sharp_class *parentClass, sharp_class *withClass, Ast *ast);
void pre_process_enum_field(sharp_class*, Ast*);

#endif //SHARP_ENUM_PREPROCESSOR_H
