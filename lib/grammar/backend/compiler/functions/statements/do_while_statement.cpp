//
// Created by bknun on 6/19/2022.
//

#include "do_while_statement.h"
#include "../../../types/types.h"
#include "../function_compiler.h"
#include "../../expressions/expression.h"

void compile_do_while_statement(Ast *ast, operation_schema *scheme, bool *controlPaths) {
    stringstream ss;
    sharp_label *beginLabel, *endLabel;
    operation_schema *subScheme = new operation_schema();
    subScheme->schemeType = scheme_do_while;

    set_internal_label_name(ss, "do_while_begin", uniqueId++)
    beginLabel = create_label(ss.str(), &current_context, ast, subScheme);
    set_internal_label_name(ss, "do_while_end", uniqueId++)
    endLabel = create_label(ss.str(), &current_context, ast, subScheme);

    create_set_label_operation(subScheme, beginLabel);

    compile_block(ast->getSubAst(ast_block), subScheme, do_while_block, beginLabel, endLabel);
    current_context.blockInfo.reachable = !current_context.blockInfo.reachable;

    expression cond;
    compile_expression(cond, ast->getSubAst(ast_expression));
    if(!current_context.blockInfo.reachable && (cond.type != type_bool && !cond.type._bool) && (cond.type != type_integer && !cond.type.integer)) {
        current_context.blockInfo.reachable = true;
    }

    if(!is_evaluable_type(cond.type)) {
        current_file->errors->createNewError(GENERIC, ast->line, ast->col, "do while loop condition of type `" + type_to_str(cond.type) + "` must evaluate to true or false");
    }

    create_get_value_operation(subScheme, &cond.scheme, false, false);
    create_jump_if_true_operation(subScheme, beginLabel);

    create_set_label_operation(subScheme, endLabel);
    add_scheme_operation(scheme, subScheme);
}
