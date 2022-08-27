//
// Created by BNunnally on 9/28/2021.
//

#include "base_expression.h"
#include "../../../../taskdelegator/task_delegator.h"
#include "../../../types/sharp_class.h"
#include "self_expression.h"
#include "../../../../compiler_info.h"

void compile_base_expression(expression *e, Ast *ast) {
    sharp_class *primary
            = get_primary_class(&currThread->currTask->file->context);
    sharp_class *base = compile_base_class(ast->getSubAst(ast_base_utype));
    compile_self_expression(primary, base, e, ast);
}

sharp_class* compile_base_class(Ast *ast) {
    sharp_class *primary
        = get_primary_class(&currThread->currTask->file->context);
    if(ast != NULL) {
        sharp_type baseType = resolve(ast->getSubAst(ast_base_utype)->getSubAst(ast_utype));
        if(baseType.type == type_class) {
            if(is_implicit_type_match(primary, baseType._class)) {
                return baseType._class;
            } else {
                create_new_error(GENERIC, ast->line, ast->col, "class `" + baseType._class->fullName
                              + "` is not a base class of primary class `" + primary->fullName + "`.");
            }
        } else {
            create_new_error(GENERIC, ast->line, ast->col, "unexpected type of `" + type_to_str(baseType)
                          + "`, instead of `class` type.");
        }
    } else {
        if(primary->baseClass != NULL)
            return primary->baseClass;
        else {
            create_new_error(GENERIC, ast->line, ast->col, "class `" + primary->fullName
                + "` does not contain a base class.");
        }
    }

    return primary;
}