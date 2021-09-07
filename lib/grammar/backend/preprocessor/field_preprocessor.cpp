//
// Created by BNunnally on 9/7/2021.
//

#include "field_preprocessor.h"
#include "../types/sharp_class.h"
#include "../types/sharp_field.h"
#include "../astparser/ast_parser.h"
#include "../../settings/settings.h"
#include "../../taskdelegator/task_delegator.h"

void pre_process_field(sharp_class *parentClass, Ast *ast) {

    uInt flags = flag_none;
    if(ast->hasSubAst(ast_access_type)) {
        flags = parse_access_flags(
                flag_public
                | flag_private | flag_protected | flag_static
                | flag_local | flag_const,
                "field", parentClass,
                ast->getSubAst(ast_access_type)
        );


        if(!check_flag(flags, flag_public) && !check_flag(flags, flag_private)
           && !check_flag(flags, flag_protected))
            flags |= flag_public;
    } else flags |= flag_public;

    if(check_flag(parentClass->flags, flag_global)) {
        if (!check_flag(flags, flag_static)) {
            flags |= flag_static;
        } else {
            create_new_warning(GENERIC, __w_access, ast->line, ast->col,
                               "`static` access specifier is added by default on global fields");
        }
    }

    pre_process_field(parentClass, flags, ast);
    if(ast->hasSubAst(ast_variable_decl)) {
        long startAst = 0;
        for(Int i = 0; i < ast->getSubAstCount(); i++) {
            if(ast->getSubAst(i)->getType() == ast_variable_decl)
            {
                startAst = i;
                break;
            }
        }

        for(Int i = startAst; i < ast->getSubAstCount(); i++) {
            pre_process_field(parentClass, flags, ast->getSubAst(i));
        }
    }
}


void pre_process_field(sharp_class *owner, uInt flags, Ast *ast) {
    string name = ast->getToken(0).getValue();
    field_type ft = normal_field;
    if(name == "thread_local") {
        ft = tls_field;
        name = ast->getToken(1).getValue();
        flags |= flag_static;
    }

    create_field(
            currThread->currTask->file,
            owner, name, flags, sharp_type(),
            ft, ast
    );
}