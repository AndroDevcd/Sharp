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

    // If the types are not the same we need to check to see if they can be assigned to a single object value
    if(!is_explicit_type_match(nullableExpression.type, fallbackExpression.type)
        && !is_implicit_type_match(nullableExpression.type, fallbackExpression.type, exclude_all)) {
        sharp_type objectType(type_object, nullableExpression.type.nullable || fallbackExpression.type.nullable);

        if(!(is_implicit_type_match(objectType, nullableExpression.type, exclude_all)
            && is_implicit_type_match(objectType, fallbackExpression.type, exclude_all))) {
            currThread->currTask->file->errors->createNewError(INTERNAL_ERROR, ast->line, ast->col,
                                  " expressions are not compatible and cannot be converted to type `object`, please check types and try again.");
        } else {
            e->type.copy(objectType.type);
        }
    } else {
        e->type.copy(nullableExpression.type);
    }

    create_null_fallback_operation(&e->scheme, &nullableExpression.scheme,
            &fallbackExpression.scheme);
}