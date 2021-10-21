//
// Created by BNunnally on 10/17/2021.
//

#include "lambda_expression.h"
#include "../../../types/sharp_field.h"
#include "../../../astparser/ast_parser.h"
#include "../../../../compiler_info.h"
#include "../../../types/types.h"
#include "../../../postprocessor/function_processor.h"
#include "../expression.h"

void compile_lambda_expression(expression *e, Ast *ast) {
    List<sharp_field*> params;
    if(ast->hasSubAst(ast_lambda_arg_list)) {
        parse_lambda_arg_list(params, ast->getSubAst(ast_lambda_arg_list));


        for(Int i = 0; i < params.size(); i++) {
            if(params.get(i)->type.type != type_any) {
                sharp_type resolvedType;
                resolve(params.get(i)->type, resolvedType,
                        false, resolve_hard_type, params.get(i)->ast);

                params.get(i)->type.copy(resolvedType);
            }
        }
    }


    sharp_type returnType, noType;
    stringstream name;
    name << anonymous_func_prefix << uniqueId++;
    string functionName = name.str();

    GUARD(globalLock)
    sharp_class *primaryClass = get_primary_class(&current_file->context);

    if(primaryClass == NULL)
        primaryClass = resolve_class(currModule, global_class_name, false, false);

    if(create_function(primaryClass, flag_public | flag_static,
            normal_function, functionName, false,
            params, noType, ast)) {
        sharp_function *anon_func = primaryClass->functions.last();

        if(ast->hasSubAst(ast_method_return_type)) {
            Ast *returnTypeAst = ast->getSubAst(ast_method_return_type);

            if(returnTypeAst->hasToken("nil")) {
                returnType.type = type_nil;
            } else {
                returnType = resolve(returnTypeAst->getSubAst(ast_utype));
            }

        } else if(ast->hasSubAst(ast_expression)) {
            create_context(anon_func->owner);
            create_context(anon_func);
            expression expr;
            compile_expression(expr, ast->getSubAst(ast_expression));
            returnType.copy(expr.type);

            anon_func->copy_scheme(&expr.scheme);
            delete_context();
            delete_context();
        } else {
            returnType.type = type_nil;
        }

        validate_function_type(true, anon_func, returnType,
                               NULL, anon_func->ast);
        if(ast->hasSubAst(ast_block)) {
            if(current_file->stage < pre_compilation_finished_state) {
                primaryClass->uncompiledLambdas.add(anon_func);
            } else {
                // todo: process block
            }
        }

        e->type.type = type_lambda_function;
        e->type.fun = anon_func;

        create_get_static_function_address_operation(&e->scheme, anon_func);
    }
}
