//
// Created by BNunnally on 10/24/2021.
//

#include "array_expression.h"
#include "../expression.h"
#include "../../../../compiler_info.h"
#include "../../../types/types.h"
#include "post_ast_expression.h"

void compile_array_expression(expression *e, Ast *ast) {
    List<sharp_field*> params;
    List<expression*> expressions;
    List<operation_schema*> paramOperations;

    Ast *list = ast->getSubAst(ast_array_index_items);
    string name = "";
    impl_location location;
    for(Int j = 0; j < list->getSubAstCount(); j++) {
        expressions.add(new expression());
        compile_expression(*expressions.last(), list->getSubAst(j));

        if(expressions.last()->scheme.schemeType == scheme_none) {
            create_new_error(
                    GENERIC, list->getSubAst(j), "expression of type `" + type_to_str(expressions.last()->type)
                                                 + "` must evaluate to a value");
        }

        convert_expression_type_to_real_type(*expressions.last());
        params.add(new sharp_field(
                name, get_primary_class(&currThread->currTask->file->ctx), location,
                expressions.last()->type, flag_public, normal_field,
                list->getSubAst(j)
        ));

        paramOperations.add(new operation_schema(expressions.last()->scheme));
    }

    if(e->type.isArray || (e->type.type == type_field && e->type.field->type.isArray)) {
        if(expressions.singular()) {
            expression &arrayExpression = *expressions.last();
            if(e->type.type == type_field) {
                e->type.copy(e->type.field->type);
            }
            e->type.isArray = false;
            e->type.nullable = e->type.nullableItems;
            e->type.nullableItems = false;
            e->type.arrayElement = true;
            extract_value_field_from_expression(arrayExpression, "", ast, true);
            create_access_array_element_operation(&e->scheme, e->type, &arrayExpression.scheme);
        } else {
            create_new_error(
                    GENERIC, ast, "Multi-dimensional arrays are not supported.");
        }
    } else {
        sharp_class *with_class = NULL;
        if(e->type.type == type_class) {
            with_class = e->type._class;
        } else if(e->type.type == type_field
            && e->type.field->type.type == type_class) {
            with_class = e->type.field->type._class;
        }

        if(with_class != NULL) {
            expression tmp;
            string op = "[";
            compile_class_function_overload(with_class, tmp, params, paramOperations, op, ast);
            e->type.copy(tmp.type);

            for(Int i = 0; i < tmp.scheme.steps.size(); i++) {
                e->scheme.steps.add(new operation_step(*tmp.scheme.steps.get(i)));
            }
        } else {
            create_new_error(
                    GENERIC, ast,
                    "type `" + type_to_str(e->type) + "` must be either a class or array type.");
        }
    }

    compile_post_ast_expression(e, ast);
}
