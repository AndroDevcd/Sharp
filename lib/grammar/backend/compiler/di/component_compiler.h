//
// Created by BNunnally on 10/3/2021.
//

#ifndef SHARP_COMPONENT_COMPILER_H
#define SHARP_COMPONENT_COMPILER_H

#include "../../../frontend/parser/Ast.h"
#include "../../dependency/component.h"

void compile_components();
void compile_component(Ast *ast);
void compile_sub_component(
        bool lastTry,
        component *comp,
        component_representation representation,
        Ast *ast);

#endif //SHARP_COMPONENT_COMPILER_H
