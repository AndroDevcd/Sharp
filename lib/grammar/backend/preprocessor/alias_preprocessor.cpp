//
// Created by BNunnally on 9/6/2021.
//

#include "alias_preprocessor.h"
#include "../types/sharp_class.h"
#include "../../frontend/parser/Ast.h"
#include "../astparser/ast_parser.h"

void pre_process_alias(sharp_class *parentClass, Ast *ast) {
    uInt flags = flag_none;
    string name = "";

    if(ast->hasSubAst(ast_access_type)) {
        flags = parse_access_flags(
                flag_public
                | flag_private | flag_protected | flag_local,
                "alias", parentClass, ast);


        if(!check_flag(flags, flag_public) && !check_flag(flags, flag_private)
           && !check_flag(flags, flag_protected))
            flags |= check_flag(parentClass->flags, flag_global) ? flag_public : flag_private;
    } else {
        flags = check_flag(parentClass->flags, flag_global) ? flag_public : flag_private;
    }

    name = ast->getToken(0).getValue();
}
