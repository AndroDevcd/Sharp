//
// Created by BNunnally on 10/3/2021.
//

#ifndef SHARP_COMPONENT_COMPILER_H
#define SHARP_COMPONENT_COMPILER_H

#include "../../../frontend/parser/Ast.h"
#include "../../dependency/component.h"

void compile_components();
void compile_component(Ast *ast);
void compile_type_definition(
        bool lastTry,
        component *comp,
        type_definition_rule rule,
        Ast *ast);

#endif //SHARP_COMPONENT_COMPILER_H
