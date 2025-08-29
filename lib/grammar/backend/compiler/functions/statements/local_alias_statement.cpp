//
// Created by bknun on 6/11/2022.
//

#include "local_alias_statement.h"
#include "../../../../../../stdimports.h"
#include "../../../../../core/access_flag.h"
#include "../../../../compiler_info.h"
#include "../../../types/types.h"
#include "../../../astparser/ast_parser.h"
#include "../../../preprocessor/class_preprocessor.h"

void compile_local_alias_statement(Ast *ast, operation_schema *scheme) {
    uInt flags;
    string name = "";

    if(ast->hasSubAst(ast_access_type)) {
        flags = parse_access_flags(
                flag_const,
                "alias", current_context.functionCxt->owner,
                ast->getSubAst(ast_access_type)
        );
    }
    flags |= flag_public;

    name = ast->getToken(0).getValue();
    check_decl_conflicts(ast, current_context.functionCxt->owner, "alias", name);

    auto *alias = create_local_alias(current_file, &current_context, name, flags, ast);
    alias->type = resolve(alias->ast->getSubAst(ast_utype));
}
