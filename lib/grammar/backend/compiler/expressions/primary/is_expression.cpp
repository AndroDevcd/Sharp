//
// Created by BNunnally on 10/22/2021.
//

#include "is_expression.h"
#include "../../../types/types.h"
#include "../expression.h"
#include "../../../../compiler_info.h"
#include "../../../../settings/settings.h"

void compile_is_expression(expression *e, Ast *ast) {
    sharp_type castType = resolve(ast->getSubAst(ast_utype));
    compile_expression(*e, ast->getSubAst(ast_expression));

    if(is_explicit_type_match(castType, e->type)
        || is_implicit_type_match(castType, e->type, exclude_all)
        || (get_class_type(e->type) != NULL && get_class_type(castType) != NULL
            && is_implicit_type_match(e->type, castType, exclude_all))) {
        e->type.type = type_bool;
        e->type._bool = true;
        create_get_bool_constant_operation(&e->scheme, e->type._bool);
    } else {
        if(is_implicit_type_match(e->type, castType, exclude_all)) {
            create_is_operation(&e->scheme, &castType);
        } else {
            e->type.type = type_bool;
            e->type._bool = false;
            create_get_bool_constant_operation(&e->scheme, e->type._bool);
        }
    }
}
