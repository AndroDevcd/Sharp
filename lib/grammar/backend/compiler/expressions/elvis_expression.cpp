//
// Created by BNunnally on 11/30/2021.
//

#include "elvis_expression.h"
#include "../../types/types.h"
#include "expression.h"
#include "../../../taskdelegator/task_delegator.h"
#include "../../../compiler_info.h"
#include "../../../settings/settings.h"

void compile_elvis_expression(expression *e, Ast *ast) {
    expression nullableExpression;
    compile_expression(nullableExpression, ast->getSubAst(0));
    convert_expression_type_to_real_type(nullableExpression);

    expression fallbackExpression;
    compile_expression(fallbackExpression, ast->getSubAst(1));
    convert_expression_type_to_real_type(fallbackExpression);

    if(!nullableExpression.type.nullable) {
        create_new_warning(GENERIC, __w_null, ast->line, ast->col,
                           "expression of type `" + type_to_str(nullableExpression.type) + "` was found to be non nullable.");
    }

    create_null_fallback_operation(&e->scheme, &nullableExpression.scheme,
            &fallbackExpression.scheme);
}