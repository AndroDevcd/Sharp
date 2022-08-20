//
// Created by BNunnally on 9/26/2021.
//

#include "self_expression.h"
#include "../expression.h"
#include "../../../context/context.h"
#include "../../../../taskdelegator/task_delegator.h"
#include "dot_notation_call_expression.h"
#include "../../../types/sharp_function.h"
#include "../../../types/types.h"
#include "../../../../compiler_info.h"
#include "post_ast_expression.h"

void compile_self_expression(expression *e, Ast *ast) {
    context &ctx = currThread->currTask->file->context;
    sharp_class *primaryClass = get_primary_class(&ctx);

    compile_self_expression(primaryClass, primaryClass, e, ast);
}

void compile_self_expression(sharp_class *primaryClass, sharp_class *instanceClass, expression *e, Ast *ast) {
    context &ctx = currThread->currTask->file->context;
    sharp_function *fun = get_primary_function(&ctx);

    if(fun != NULL && !check_flag(fun->flags, flag_static)
       && ctx.type == block_context && ctx.functionCxt != fun) {
        sharp_class *closure_class = create_closure_class(
                currThread->currTask->file, currModule, fun,
                ast);
        sharp_field *closure = create_closure_field(closure_class, "__@self",
                                                    sharp_type(primaryClass), ast);
        sharp_field *staticClosureRef = create_closure_field(
                resolve_class(currModule, global_class_name, false, false),
                "closure_ref_" + fun->fullName,
                sharp_type(closure_class),
                ast
        );

        fun->closure = staticClosureRef;

        create_dependency(fun, closure);
        create_dependency(fun, staticClosureRef);
        create_static_field_access_operation(&e->scheme, staticClosureRef);
        create_instance_field_access_operation(&e->scheme, closure);
    } else {
        if(ctx.isStatic) {
            currThread->currTask->file->errors->createNewError(GENERIC, ast,
                     "cannot access self from static context.");
        } else {
            e->scheme.schemeType = scheme_get_primary_class_instance;
            create_get_primary_instance_class(&e->scheme, instanceClass);
        }
    }

    e->type.type = type_class;
    e->type._class = instanceClass;

    if(ast->hasToken(PTR)) {
        create_context(instanceClass, false);
        compile_dot_notation_call_expression(
                e, NULL, true, ast->getSubAst(ast_dotnotation_call_expr));
        delete_context();
    } else {
        compile_post_ast_expression(e, ast);
    }
}

