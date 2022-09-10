//
// Created by bknun on 8/3/2022.
//

#include "force_non_null_expression.h"
#include "../expression.h"
#include "../../../../frontend/ErrorManager.h"
#include "../../../../compiler_info.h"
#include "../../../../settings/settings.h"
#include "../../../types/function_type.h"

void force_null_safety_check(operation_schema *outScheme, Int endLabel, Ast *ast) {
    APPLY_TEMP_SCHEME(1, *outScheme,
       create_get_value_operation(&scheme_1, outScheme);
       create_check_null_operation(&scheme_1);
       create_jump_if_true_operation(&scheme_1, endLabel);
       create_set_current_data_as_object_operation(&scheme_1);

       scheme_1.schemeType = scheme_get_value;
       outScheme->free();
    )
}

void compile_force_non_null_expression(expression *e, Ast *ast) {
    compile_expression(*e, ast->getSubAst(ast_expression));

    sharp_type actualType = get_real_type(e->type);
    if(actualType.nullable) {
        e->type.copy(actualType);
        e->type.nullable = false;
    } else {
        if(!is_object_type(actualType) && get_class_type(actualType) == NULL
            && !actualType.isArray) {
            create_new_error(GENERIC, ast, "cannot use operator `!!` with type `"
                + type_to_str(e->type) + "`");
        } else {
            create_new_warning(GENERIC, __w_cast, ast, "redundant use of operator `!!` with non-nullable type `"
                + type_to_str(e->type) + "`");
        }
    }
}
