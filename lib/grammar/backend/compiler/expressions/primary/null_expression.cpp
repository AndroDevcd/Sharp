//
// Created by BNunnally on 9/29/2021.
//

#include "null_expression.h"
#include "../expression.h"

void compile_null_expression(expression *e, Ast *ast) {
    e->type.type = type_null;
    e->type.nullable = true;

    create_null_value_operation(&e->scheme);
}
