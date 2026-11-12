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
#include "../../functions/function_compiler.h"

/**
 * Does the body of an implicit lambda (`xs.filter { it > 3 }`) use the name
 * `it`? Lambdas nested inside it that are implicit themselves are not looked
 * into: their `it` is their own.
 */
static bool uses_implicit_parameter(Ast *ast) {
    for(Int i = 0; i < ast->getTokenCount(); i++) {
        Token &token = ast->getToken(i);
        if(token.getId() == IDENTIFIER && token.getValue() == "it")
            return true;
    }

    for(Int i = 0; i < ast->getSubAstCount(); i++) {
        Ast *sub = ast->getSubAst(i);
        if(sub->getType() == ast_lambda_function && sub->hasToken("@implicit"))
            continue;
        if(uses_implicit_parameter(sub))
            return true;
    }
    return false;
}

void compile_lambda_expression(expression *e, Ast *ast) {
    List<sharp_field*> params;

    // A trailing lambda written without `->` (see parser::parseTrailingLambda):
    // it has one parameter called `it` when its body uses that name, none
    // otherwise. The type of `it` comes from the function type the lambda is
    // passed to, like the type of any untyped lambda parameter.
    if(ast->hasToken("@implicit") && ast->hasSubAst(ast_block)
       && uses_implicit_parameter(ast->getSubAst(ast_block))) {
        impl_location location(currThread->currTask->file, ast);
        string name = "it";
        sharp_type type;
        type.type = type_any;
        params.add(new sharp_field(name, NULL, location, type, flag_public, normal_field, ast));
    }

    if(ast->hasSubAst(ast_lambda_arg_list)) {
        parse_lambda_arg_list(params, ast->getSubAst(ast_lambda_arg_list));


        for(Int i = 0; i < params.size(); i++) {
            if(params.get(i)->type.type != type_any) {
                sharp_type resolvedType;
                resolve(params.get(i)->type, resolvedType,
                        false, resolve_hard_type, -1, params.get(i)->ast);

                params.get(i)->type.copy(resolvedType);
            }
        }
    }


    sharp_type returnType, noType;
    stringstream name;
    name << anonymous_func_prefix << uniqueId++;
    string functionName = name.str();

    GUARD(globalLock)
    sharp_class *primaryClass = get_primary_class(&current_file->ctx);

    if(primaryClass == NULL)
        primaryClass = resolve_class(currModule, global_class_name, false, false);

    sharp_function *anon_func = NULL;
    if(create_function(primaryClass, flag_public | flag_static,
            normal_function, functionName, false,
            params, noType, ast, anon_func)) {
        create_dependency(anon_func);

        anon_func->locals.addAll(anon_func->parameters);
        if(ast->hasSubAst(ast_method_return_type)) {
            Ast *returnTypeAst = ast->getSubAst(ast_method_return_type);

            if(returnTypeAst->hasToken("nil")) {
                returnType.type = type_nil;
            } else {
                returnType = resolve(returnTypeAst->getSubAst(ast_utype));
            }

            validate_function_type(true, anon_func, returnType,
                                   NULL, anon_func->ast);
        } else if(ast->hasSubAst(ast_expression)) {
            create_context(anon_func->owner);
            create_context(anon_func);
            create_block(&current_context, normal_block);

            expression expr;
            compile_expression(expr, ast->getSubAst(ast_expression));
            returnType.copy(expr.type);

            anon_func->copy_scheme(&expr.scheme);
            delete_block();
            delete_context();
            delete_context();

            validate_function_type(true, anon_func, returnType,
                                   NULL, anon_func->ast);
        } else {
            anon_func->returnType.type = type_any;
        }

        if(ast->hasSubAst(ast_block)) {
            if(current_file->stage < pre_compilation_state) {
                primaryClass->uncompiledLambdas.add(anon_func);
            } else if(is_fully_qualified_function(anon_func)){
                compile_function(anon_func, anon_func->ast);
            }
        }

        e->type.type = type_lambda_function;
        e->type.fun = anon_func;

        create_get_static_function_address_operation(&e->scheme, anon_func);
    }
}
