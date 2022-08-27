//
// Created by BNunnally on 9/26/2021.
//

#include "dot_notation_call_expression.h"
#include "../expression.h"
#include "../../../dependency/dependancy.h"
#include "../../../../taskdelegator/task_delegator.h"
#include "post_ast_expression.h"
#include "../../../../compiler_info.h"

void compile_dot_notation_call_expression(
        expression *e,
        sharp_class *with_class,
        bool fromSelf,
        Ast *ast) {

    uInt filter = fromSelf ? resolve_inner_class_type : resolve_all;
    operation_schema *scheme;
    if(with_class == NULL) {
        scheme = &e->scheme;
    } else {
        scheme = new operation_schema();
    }

    Ast *dotNotationAst = ast->getType() == ast_dotnotation_call_expr ? ast : ast->getSubAst(ast_dotnotation_call_expr);
    if(dotNotationAst->hasSubAst(ast_dot_fn_e)) {
        e->type.copy(resolve(dotNotationAst, filter, scheme, with_class));
    } else {
        e->type.copy(resolve(dotNotationAst->getSubAst(ast_utype), filter,
                             scheme, with_class));
    }

    if(scheme->schemeType == scheme_none) {
        create_new_error(
                GENERIC, dotNotationAst, "expression of type `"
                              + type_to_str(e->type) + "` must evaluate to a value");
    }

    if(with_class != NULL) {
        e->scheme.steps.add(new operation_step(operation_get_value, scheme));
        delete scheme;
    }

    compile_post_ast_expression(e, dotNotationAst);
}