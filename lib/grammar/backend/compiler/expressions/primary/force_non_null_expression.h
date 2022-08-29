//
// Created by bknun on 8/3/2022.
//

#ifndef SHARP_FORCE_NON_NULL_EXPRESSION_H
#define SHARP_FORCE_NON_NULL_EXPRESSION_H

#include "../../../../frontend/parser/Ast.h"
#include "../../../operation/operation.h"

struct expression;

void force_null_safety_check(operation_schema *outScheme, Int endLabel, Ast *ast);
void force_non_null_check(operation_schema *outScheme, Ast *ast);
void compile_force_non_null_expression(expression *e, Ast *ast);

#endif //SHARP_FORCE_NON_NULL_EXPRESSION_H
