//
// Created by BNunnally on 9/10/2021.
//

#include "alias_processor.h"
#include "../dependency/dependancy.h"
#include "../types/sharp_alias.h"
#include "../types/sharp_class.h"
#include "../../compiler_info.h"

void process_alias(sharp_class *with_class, Ast *ast) {
    string name = ast->getToken(0).getValue();

    sharp_alias *alias = resolve_alias(name, with_class);

    process_alias(alias);
}

void process_alias(sharp_alias *alias) {
    GUARD(globalLock)
    alias->type = resolve(alias->ast->getSubAst(ast_utype));
}
