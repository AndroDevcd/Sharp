//
// Created by bknun on 6/25/2022.
//

#include "lock_statement.h"
#include "../../../../compiler_info.h"
#include "../../../types/types.h"
#include "../../expressions/expression.h"
#include "../function_compiler.h"

void compile_lock_statement(Ast *ast, operation_schema *scheme) {
    List<operation_schema*> lockSchemes;
    stored_block_info* info;
    operation_schema *subScheme = new operation_schema();
    subScheme->schemeType = scheme_lock;

    expression cond;
    compile_expression(cond, ast->getSubAst(ast_expression));

    if(get_class_type(cond.type) != NULL || is_object_type(cond.type)
        || cond.type.isArray) {
        create_lock_operation(subScheme, &cond.scheme);

        compile_block(ast->getSubAst(ast_block), subScheme, lock_block, NULL, NULL, &cond.scheme);
        add_scheme_operation(scheme, subScheme);
    } else {
        current_file->errors->createNewError(GENERIC, ast->line, ast->col, "attempt to lock non-lockable object of type `" +
                type_to_str(cond.type) + "`.");
    }
}
