//
// Created by bknun on 6/25/2022.
//

#include "lock_statement.h"
#include "../../../../compiler_info.h"
#include "../../../types/types.h"
#include "../../expressions/expression.h"
#include "../function_compiler.h"
#include "../../compiler.h"

void compile_lock_statement(Ast *ast, operation_schema *scheme, bool *controlPaths) {
    List<operation_schema*> lockSchemes;
    stored_block_info* info;
    operation_schema *subScheme = new operation_schema();

    expression cond;
    compile_expression(cond, ast->getSubAst(ast_expression));

    if(get_class_type(cond.type) != NULL || is_object_type(cond.type)
        || cond.type.isArray) {
        create_lock_operation(subScheme, &cond.scheme);

        if(compile_block(ast->getSubAst(ast_block), subScheme, lock_block, NULL, NULL, ast->getSubAst(ast_expression))) {
            controlPaths[MAIN_CONTROL_PATH] = true;
        }

        recompile_cond_expression(cond, ast->getSubAst(ast_expression));
        create_unlock_operation(subScheme, &cond.scheme);
        subScheme->schemeType = scheme_lock;
        add_scheme_operation(scheme, subScheme);
    } else {
        create_new_error(GENERIC, ast->line, ast->col, "attempt to lock non-lockable object of type `" +
                type_to_str(cond.type) + "`.");
    }
    delete subScheme;
}
