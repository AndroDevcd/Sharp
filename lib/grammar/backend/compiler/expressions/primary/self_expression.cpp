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
#include "../../../dependency/closure_capture.h"

void compile_self_expression(expression *e, Ast *ast, Int endLabel) {
    context &ctx = currThread->currTask->file->ctx;
    sharp_class *primaryClass = get_primary_class(&ctx);

    compile_self_expression(primaryClass, NULL, e, ast, endLabel);
}

/**
 * `self` written inside a lambda: the instance function the lambda (or the
 * lambdas around it) was written in, NULL when the code is not in a lambda or
 * the function around the lambdas is static.
 *
 *      class counter {
 *          def bump() { run({ -> self->count++; }); }      // -> counter.bump
 *      }
 */
static sharp_function* instance_function_around_lambda(context &ctx) {
    string lambdaPrefix = anonymous_func_prefix;
    if(ctx.functionCxt == NULL || ctx.functionCxt->name.find(lambdaPrefix) != 0)
        return NULL;

    for(Int i = (Int)ctx.storedItems.size() - 1; i >= 0; i--) {
        sharp_function *fun = ctx.storedItems.get(i)->functionCxt;
        if(fun == NULL || fun->name.find(lambdaPrefix) == 0)
            continue;   // a class, a block, or another lambda on the way out

        return check_flag(fun->flags, flag_static) ? NULL : fun;
    }
    return NULL;
}

void compile_self_expression(sharp_class *primaryClass, sharp_class *instanceClass, expression *e, Ast *ast, Int endLabel) {
    context &ctx = currThread->currTask->file->ctx;
    sharp_function *fun = instance_function_around_lambda(ctx);

    if(fun != NULL) {
        // `self` inside a lambda of an instance function: the lambda is a
        // static function and has no instance of its own. The instance is
        // captured like a local of the function (closure_capture.h), and
        // what follows `self` is looked up on that captured object.
        // (This used to be an error, "cannot access self from static
        //  context": the test that was here never held.)
        capture_enclosing_self(fun, fun->owner, ctx, &e->scheme, ast);

        e->type.type = type_class;
        e->type._class = fun->owner;

        if(ast->hasToken(PTR)) {
            create_post_access_operation(&e->scheme);
            compile_dot_notation_call_expression(
                    e, fun->owner, false, endLabel, ast->getSubAst(ast_dotnotation_call_expr));
        } else {
            compile_post_ast_expression(e, ast);
        }
        return;
    } else {
        if(ctx.isStatic) {
            create_new_error(GENERIC, ast,
                     "cannot access self from static context.");
        } else {
            e->scheme.schemeType = scheme_get_primary_class_instance;
            create_get_primary_instance_class(&e->scheme, instanceClass);
        }
    }

    e->type.type = type_class;
    e->type._class = primaryClass;

    if(ast->hasToken(PTR)) {
//        create_context(instanceClass, false);
        compile_dot_notation_call_expression(
                e, instanceClass, true, endLabel, ast->getSubAst(ast_dotnotation_call_expr));
//        delete_context();
    } else {
        compile_post_ast_expression(e, ast);
    }
}

