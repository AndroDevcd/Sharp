//
// Created by bknun on 8/3/2022.
//

#include "force_non_null_expression.h"
#include "../expression.h"
#include "../../../../frontend/ErrorManager.h"
#include "../../../../compiler_info.h"
#include "../../../../settings/settings.h"

void compile_force_non_null_expression(expression *e, Ast *ast) {
    compile_expression(*e, ast->getSubAst(ast_expression));

    sharp_type actualType = get_real_type(e->type);
    if(actualType.nullable) {
        if(e->type == type_field) {
            APPLY_TEMP_SCHEME(1, e->scheme,
                // todo: allso enforce non-nullable check on !!. as well
                create_get_value_operation(&scheme_1, &e->scheme); // todo: add ensure non-null check here or throw
                e->scheme.free();
            )

            e->type.copy(actualType);
        }

        e->type.nullable = false;
    } else {
        if(!is_object_type(actualType) && get_class_type(actualType) == NULL
            && !actualType.isArray) {
            current_file->errors->createNewError(GENERIC, ast, "cannot use operator `!!` with type `"
                + type_to_str(e->type) + "`");
        } else {
            create_new_warning(GENERIC, __w_cast, ast, "redundant use of operator `!!` with non-nullable type `"
                + type_to_str(e->type) + "`");
        }
    }
}
