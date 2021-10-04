//
// Created by BNunnally on 9/29/2021.
//

#include "new_expression.h"
#include "../../../types/sharp_type.h"
#include "../../../dependency/dependancy.h"
#include "../expression.h"

void compile_new_expression(expression *e, Ast *ast) {
    sharp_type newType = resolve(ast->getSubAst(ast_utype));

    if(ast->hasSubAst(ast_array_expression)) {
        compile_new_array_expression(&newType, e, ast->getSubAst(ast_array_expression));
    }
}

void compile_new_array_expression(sharp_type *newType, expression *e, Ast *ast) {
    expression arraySize;
    compile_expression(*e, ast);

    e->type.copy(*newType);
    e->type.isArray = true;

    if(e->type.type == type_class)
        create_new_class_array_operation(&e->scheme, &arraySize.scheme, e->type._class);
    else if(e->type.type >= type_int8 && e->type.type <= type_var)
        create_new_number_array_operation(&e->scheme, &arraySize.scheme, e->type.type);
    else if(e->type.type == type_object)
        create_new_object_array_operation(&e->scheme, &arraySize.scheme);
    else {
        // error
    }
}
