//
// Created by BNunnally on 12/1/2021.
//

#ifndef SHARP_ASSIGN_EXPRESSION_H
#define SHARP_ASSIGN_EXPRESSION_H

#include "../../../frontend/parser/Ast.h"

struct expression;

void compile_assign_expression(expression *e, Ast *ast);

/** is `left` a field with a getter and a setter, used from outside both? */
bool is_property_target(expression &left);

/**
 * `p.value++` / `p.value--` when `*e` is a property: compiled as
 * "call the setter with (getter + 1)". Returns false, and changes nothing,
 * when `*e` is not a number property or `op` is another operator.
 */
bool compile_property_step(expression *e, Ast *ast, const string &op);

#endif //SHARP_ASSIGN_EXPRESSION_H
