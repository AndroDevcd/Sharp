//
// Created by bknun on 9/1/2022.
//

#ifndef SHARP_OBFUSCATE_COMPILER_H
#define SHARP_OBFUSCATE_COMPILER_H

#include "../../frontend/parser/Ast.h"

struct sharp_class;

enum obfuscation_modifier {
    modifier_none=0,
    modifier_keep,
    modifier_keep_inclusive,
    modifier_obfuscate,
    modifier_obfuscate_inclusive
};

void compile_obfuscation_declaration(Ast *ast);
void obfuscate_class(sharp_class *sc, obfuscation_modifier modifier, Ast *ast);

#endif //SHARP_OBFUSCATE_COMPILER_H
