//
// Created by BNunnally on 10/21/2021.
//

#include "sizeof_expression.h"
#include "../expression.h"
#include "../../../../compiler_info.h"
#include "post_ast_expression.h"

void compile_sizeof_expression(expression *e, Ast *ast) {
    expression valueExpr;
    compile_expression(valueExpr, ast->getSubAst(ast_expression));

    auto type = get_real_type(valueExpr.type);
    if(type.type == type_string
        || type.type == type_class
        || type.isArray) {
        create_sizeof_operation(&e->scheme, &valueExpr.scheme);
        e->type.type = type_int32;
    } else {
        create_new_error(GENERIC, ast,
                "cannot get sizeof from expression of type `" + type_to_str(valueExpr.type) + "`");
    }

    compile_post_ast_expression(e, ast);
}
