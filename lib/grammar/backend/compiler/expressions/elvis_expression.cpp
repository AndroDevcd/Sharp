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
    if(ast->line == 174 && ast->col == 23 && ast->sub_asts.get(0)->col == 20) {
        int i = 0;
    }
    compile_expression(nullableExpression, ast->getSubAst(0));
    convert_expression_type_to_real_type(nullableExpression);

    expression fallbackExpression;
    compile_expression(fallbackExpression, ast->getSubAst(1));
    convert_expression_type_to_real_type(fallbackExpression);

    if(!nullableExpression.type.nullable) {
        create_new_warning(GENERIC, __w_null, ast->line, ast->col,
                           "expression of type `" + type_to_str(nullableExpression.type) + "` was found to be non nullable.");
    }

    // If the types are not the same we need to check to see if they can be assigned to a single object value
    Int match_result;
    sharp_function *constructor;
    if((match_result = is_implicit_type_match(nullableExpression.type, fallbackExpression.type, constructor_only, constructor)) != no_match_found) {
        e->type.copy(get_real_type(nullableExpression.type));
        e->type.nullable = !fallbackExpression.type.nullable || !nullableExpression.type.nullable;

        if(match_result == match_constructor) {
            operation_schema scheme;
            compile_initialization_call(ast, constructor, fallbackExpression, &scheme);
            fallbackExpression.scheme.free();
            fallbackExpression.scheme.copy(scheme);
        }
    } else {
        create_new_error(INTERNAL_ERROR, ast->line, ast->col,
                         " expressions are not compatible and cannot be converted to type `" +
                         type_to_str(nullableExpression.type) + "`,  to type `" + type_to_str(fallbackExpression.type) + "`.");
    }

    create_null_fallback_operation(&e->scheme, &nullableExpression.scheme,
            &fallbackExpression.scheme);
}