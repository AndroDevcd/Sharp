//
// Created by bknun on 6/19/2022.
//

#include "throw_statement.h"
#include "../../expressions/expression.h"
#include "../../compiler.h"
#include "../../../../compiler_info.h"
#include "../../../types/sharp_module.h"
#include "../../../types/types.h"

void compile_throw_statement(Ast *ast, operation_schema *scheme, bool *controlPaths) {
    expression exception;
    operation_schema *subScheme = new operation_schema();
    current_context.blockInfo.reachable = false;
    controlPaths[MAIN_CONTROL_PATH] = true;

    compile_expression(exception, ast->getSubAst(ast_expression));

    if(get_class_type(exception.type) != NULL) {
        string std = "std";
        sharp_class *throwable = resolve_class(get_module(std), "throwable", false, false);
        if(is_class_related_to(get_class_type(exception.type), throwable)) {
            create_throw_operation(subScheme, &exception.scheme);
        } else {
            create_new_error(GENERIC, ast->line, ast->col, "expression of type `" + type_to_str(exception.type) + "` must inherit base level exception class `std#throwable`");
        }
    } else {
        create_new_error(GENERIC, ast->line, ast->col, "expression of type `" + type_to_str(exception.type) + "` must evaluate to a class");
    }

    add_scheme_operation(scheme, subScheme);
    delete subScheme;
}