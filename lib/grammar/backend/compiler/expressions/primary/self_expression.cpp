//
// Created by BNunnally on 9/26/2021.
//

#include "self_expression.h"
#include "../expression.h"
#include "../../../context/context.h"
#include "../../../../taskdelegator/task_delegator.h"
#include "dot_notation_call_expression.h"

void compile_self_expression(expression *e, Ast *ast) {
    if(ast->hasToken(PTR)) {
        compile_dot_notation_call_expression(
                e, ast->getSubAst(ast_dotnotation_call_expr));
    } else {
        sharp_class *primaryClass // check if instance is static
            = get_primary_class(&currThread->currTask->file->context);
        e->type.type = type_class;
        e->type._class = primaryClass;
        create_get_primary_instance_class(&e->scheme, primaryClass);
    }
}