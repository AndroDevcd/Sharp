//
// Created by BNunnally on 12/1/2021.
//

#ifndef SHARP_ASSIGN_EXPRESSION_H
#define SHARP_ASSIGN_EXPRESSION_H

#include "../../../frontend/parser/Ast.h"

struct expression;

void compile_assign_expression(expression *e, Ast *ast);


#endif //SHARP_ASSIGN_EXPRESSION_H
