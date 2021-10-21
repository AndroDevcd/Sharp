//
// Created by BNunnally on 10/21/2021.
//

#include "cast_expression.h"
#include "../../../types/types.h"
#include "../../../dependency/dependancy.h"
#include "../expression.h"
#include "../../../../compiler_info.h"
#include "../../../../settings/settings.h"


void compile_cast_expression(expression *e, Ast *ast, bool compileExpression) {
    sharp_type castType = resolve(ast->getSubAst(ast_utype));

    if(compileExpression)
        compile_expression(*e, ast->getSubAst(ast_expression));

    if(castType== type_class
        || castType == type_object
        || (castType.type >= type_int8 && castType.type <= type_var)){
        if(is_explicit_type_match(castType, e->type)) {
            create_new_warning(GENERIC, __w_cast, ast, "redundant cast from `" + type_to_str(e->type)
                + "` to `" + type_to_str(castType) + "`");
        } else {
            if(!is_implicit_type_match(castType, e->type, exclude_all)) {
                current_file->errors->createNewError(GENERIC, ast, "cannot cast `" + type_to_str(e->type)
                                                           + "` to `" + type_to_str(castType) + "`");
            } else {
                create_cast_operation(&e->scheme, &castType);
            }
        }
    }  else {
        current_file->errors->createNewError(GENERIC, ast->line, ast->col, "cast expression of type `" + type_to_str(castType) + "` not allowed, must be of type `class`, `object`, or native type");
    }
}