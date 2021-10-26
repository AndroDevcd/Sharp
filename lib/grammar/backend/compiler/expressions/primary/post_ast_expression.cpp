//
// Created by BNunnally on 10/21/2021.
//

#include "post_ast_expression.h"
#include "../unary/post_inc_expression.h"
#include "cast_expression.h"
#include "is_expression.h"
#include "dot_notation_call_expression.h"
#include "../expression.h"
#include "../../../types/types.h"
#include "../../../../compiler_info.h"
#include "../../../../settings/settings.h"
#include "array_expression.h"

void compile_post_ast_expression(expression *e, Ast *ast, Int startPos) {
    for(Int i = startPos; i < ast->getSubAstCount(); i++) {
        if(ast->getSubAst(i)->getType() == ast_post_inc_e)
            compile_post_inc_expression(e, ast->getSubAst(i), false);
        else if(ast->getSubAst(i)->getType() == ast_dotnotation_call_expr) {
            sharp_class *with_class = NULL;
            if(e->type.type == type_class) {
                with_class = e->type._class;
            } else if(e->type.type == type_field
                && e->type.field->type.type == type_class) {
                with_class = e->type.field->type._class;
            }

            if(with_class != NULL) {
                Token *access = &ast->getSubAst(i)->getToken(0);
                if(access->getType() == DOT
                    && e->type.nullable) {
                    current_file->errors->createNewError(GENERIC, ast, "Unsafe use of nullable type `"
                                  + type_to_str(e->type) + "` without `?.` or `!!.`.");
                } else if(access->getType() != DOT
                          && !e->type.nullable) {
                    create_new_warning(GENERIC, __w_access, ast, "unnecessary use of`" + access->getValue()
                               + "` on non nullable type `" + type_to_str(e->type) + "`");
                }

                compile_dot_notation_call_expression(e, with_class, ast->getSubAst(i));
            } else {
                current_file->errors->createNewError(GENERIC, ast, "expected `class` type but type `"
                    + type_to_str(e->type) + "` was found.");
            }
        } else if(ast->getSubAst(i)->getType() == ast_dot_fn_e) {
            sharp_function *funPtr = NULL;
            if(e->type.type == type_function_ptr) {
                funPtr = e->type.fun;
            } else if(e->type.type == type_field
                      && e->type.field->type.type == type_function_ptr) {
                funPtr = e->type.field->type.fun;
            }

            if(funPtr != NULL) {
                List<sharp_field*> params;
                List<expression*> expressions;
                List<operation_scheme*> paramOperations;

                string name = "";
                impl_location location;
                Ast *list = ast->getSubAst(i)->getSubAst(ast_expression_list);
                for(Int j = 0; j < list->getSubAstCount(); j++) {
                    expressions.add(new expression());
                    compile_expression(*expressions.last(), list->getSubAst(j));

                    if(expressions.last()->scheme.schemeType == scheme_none
                       && expressions.last()->type != type_get_component_request) {
                        currThread->currTask->file->errors->createNewError(
                                GENERIC, list->getSubAst(j), "expression of type `" + type_to_str(expressions.last()->type)
                                    + "` must evaluate to a value");
                    }

                    params.add(new sharp_field(
                            name, get_primary_class(&currThread->currTask->file->context), location,
                            expressions.last()->type, flag_public, normal_field,
                            list->getSubAst(j)
                    ));

                    paramOperations.add(new operation_scheme(expressions.last()->scheme));
                }

                if(is_fully_qualified_function(funPtr)) {
                    if(expressions.size() == funPtr->parameters.size()) {
                        bool parametersMatch = true;
                        for(Int j = 0; j < expressions.size(); j++) {
                            expression mock;
                            mock.type.copy(expressions.get(j)->type);
                            convert_expression_type_to_real_type(list->getSubAst(j), current_file, mock);

                            if(!(is_explicit_type_match(funPtr->parameters.get(j)->type, mock.type)
                                && is_implicit_type_match(funPtr->parameters.get(j)->type, mock.type,
                                        match_initializer | match_operator_overload))) {
                                parametersMatch = false;
                                current_file->errors->createNewError(GENERIC, list->getSubAst(j), "Parameter type mismatch parameter of type `"
                                           + type_to_str(mock.type) + "` was found but `" + type_to_str(funPtr->parameters.get(j)->type) + "` was expected.");
                            }
                        }

                        if(parametersMatch) {
                            operation_scheme scheme;
                            compile_function_call(
                                    &scheme,
                                    params,
                                    paramOperations,
                                    funPtr,
                                    true,
                                    false
                            );

                            e->type.copy(funPtr->returnType);
                            e->scheme.steps.appendAll(scheme.steps);
                        }
                    } else {
                        stringstream ss;
                        ss << "Too many parameters for function call, expected `" << funPtr->parameters.size() << "` but `"
                            << expressions.size() << "` were found.";
                        current_file->errors->createNewError(GENERIC, ast, ss.str());
                    }
                } else {
                    current_file->errors->createNewError(GENERIC, ast, "cannot call non-fully qualified function `"
                                  + type_to_str(e->type) + "`.");
                }

                deleteList(params);
                deleteList(expressions);
                deleteList(paramOperations);
            } else {
                current_file->errors->createNewError(GENERIC, ast, "expected `function pointer` type but type `"
                              + type_to_str(e->type) + "` was found.");
            }
        } else if(ast->getSubAst(i)->getType() == ast_arry_e) {
            compile_array_expression(e, ast->getSubAst(i));
        } else {
            error_manager->createNewError(GENERIC, ast->getSubAst(i)->line, ast->getSubAst(i)->col, "unexpected expression of type `" + Ast::astTypeToString(ast->getSubAst(i)->getType()) + "`");
        }
    }
}
