//
// Created by BNunnally on 9/22/2021.
//

#include "minus_expression.h"
#include "../expression.h"
#include "../../../../taskdelegator/task_delegator.h"
#include "../../../types/types.h"
#include "unary_expression.h"

void compile_minus_expression(expression *e, Ast *ast) {
    string op = "-";
    compile_unary_prefix_expression(e, op, ast);
}