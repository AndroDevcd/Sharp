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
    List<operation_scheme*> paramOperations;

    Ast *list = ast->getSubAst(ast_array_index_items);
    string name = "";
    impl_location location;
    for(Int j = 0; j < list->getSubAstCount(); j++) {
        expressions.add(new expression());
        compile_expression(*expressions.last(), list->getSubAst(j));

        if(expressions.last()->scheme.schemeType == scheme_none) {
            currThread->currTask->file->errors->createNewError(
                    GENERIC, list->getSubAst(j), "expression of type `" + type_to_str(expressions.last()->type)
                                                 + "` must evaluate to a value");
        }

        convert_expression_type_to_real_type(list->getSubAst(j), current_file, *expressions.last());
        params.add(new sharp_field(
                name, get_primary_class(&currThread->currTask->file->context), location,
                expressions.last()->type, flag_public, normal_field,
                list->getSubAst(j)
        ));

        paramOperations.add(new operation_scheme(expressions.last()->scheme));
    }

    if(e->type.isArray) {
        if(expressions.singular()) {
            expression &arrayExpression = *expressions.last();
            if ((arrayExpression.type.type >= type_int8 && arrayExpression.type.type <= type_uint64)
                || arrayExpression.type.type == type_var) {
                create_access_array_element_operation(&e->scheme, &arrayExpression.scheme);
            } else {
                currThread->currTask->file->errors->createNewError(
                        GENERIC, ast, "array index expression must be numeric but was found to be of type `"
                                      + type_to_str(arrayExpression.type) + "`.");
            }
        } else {
            currThread->currTask->file->errors->createNewError(
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
            e->scheme.steps.appendAll(tmp.scheme.steps);
        } else {
            currThread->currTask->file->errors->createNewError(
                    GENERIC, ast,
                    "type `" + type_to_str(e->type) + "` must be either a class or array type.");
        }
    }

    compile_post_ast_expression(e, ast);
}
