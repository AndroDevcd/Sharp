//
// Created by BNunnally on 9/6/2021.
//

#include "alias_preprocessor.h"
#include "../types/sharp_class.h"
#include "../../frontend/parser/Ast.h"
#include "../astparser/ast_parser.h"
#include "../types/sharp_alias.h"
#include "../../taskdelegator/task_delegator.h"
#include "../../compiler_info.h"
#include "../../settings/settings.h"
#include "class_preprocessor.h"

void pre_process_alias(sharp_class *parentClass, Ast *ast) {
    uInt flags;
    string name = "";

    if(ast->hasSubAst(ast_access_type)) {
        flags = parse_access_flags(
                flag_public
                | flag_private | flag_protected | flag_local,
                "alias", parentClass,
                ast->getSubAst(ast_access_type)
        );


        if(!check_flag(flags, flag_public) && !check_flag(flags, flag_private)
           && !check_flag(flags, flag_protected))
            flags |= check_flag(parentClass->flags, flag_global) ? flag_public : flag_private;
    } else {
        flags = check_flag(parentClass->flags, flag_global) ? flag_public : flag_private;
    }

    if(!check_flag(flags, flag_static)) {
        flags |= flag_static;
    } else {
        create_new_warning(GENERIC, __w_access, ast->line, ast->col,
                           "`static` access specifier is added by default on aliases");
    }

    name = ast->getToken(0).getValue();
    check_decl_conflicts(ast, parentClass, "alias", name);
    create_alias(currThread->currTask->file, parentClass, name, flags, ast);
}
