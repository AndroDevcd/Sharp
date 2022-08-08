//
// Created by BNunnally on 10/21/2021.
//

#include "cast_expression.h"
#include "../../../types/types.h"
#include "../../../dependency/dependancy.h"
#include "../expression.h"
#include "../../../../compiler_info.h"
#include "../../../../settings/settings.h"


void compile_cast_expression(expression *e, Ast *ast) {
    sharp_type castType = resolve(ast->getSubAst(ast_utype));
    compile_expression(*e, ast->getSubAst(ast_expression));

    if(castType== type_class
        || castType == type_object
        || (castType.type >= type_int8 && castType.type <= type_var)){
        if(is_explicit_type_match(castType, e->type)) {
            create_new_warning(GENERIC, __w_cast, ast, "redundant cast from `" + type_to_str(e->type)
                + "` to `" + type_to_str(castType) + "`");
        } else {
            if((get_real_type(e->type) == type_object && is_implicit_type_match(e->type, castType, exclude_all))
                || (get_real_type(e->type) == type_object && get_class_type(castType) != NULL)
                || is_implicit_type_match(castType, e->type, exclude_all)
                || (get_class_type(e->type) != NULL && get_class_type(castType)
                    && is_implicit_type_match(get_class_type(e->type), get_class_type(castType)))) {

                if(!(castType.type == type_object
                    || (get_class_type(e->type) != NULL && get_class_type(castType)
                        && is_implicit_type_match(get_class_type(e->type), get_class_type(castType))))) {
                    create_cast_operation(&e->scheme, &castType);
                }

            } else {
                current_file->errors->createNewError(GENERIC, ast, "cannot cast `" + type_to_str(e->type)
                                                                   + "` to `" + type_to_str(castType) + "`");
            }
        }

        e->type.copy(castType);
    }  else {
        current_file->errors->createNewError(GENERIC, ast->line, ast->col, "cast expression of type `" + type_to_str(castType) + "` not allowed, must be of type `class`, `object`, or native type");
    }
}