//
// Created by bknun on 6/19/2022.
//

#include "while_statement.h"
#include "../../../types/types.h"
#include "../../expressions/expression.h"
#include "../function_compiler.h"

void compile_while_statement(Ast *ast, operation_schema *scheme, bool *controlPaths) {
    stringstream ss;
    sharp_label *beginLabel, *endLabel;
    operation_schema *subScheme = new operation_schema();
    subScheme->schemeType = scheme_while;

    set_internal_label_name(ss, "while_begin", uniqueId++)
    beginLabel = create_label(ss.str(), &current_context, ast, subScheme);
    set_internal_label_name(ss, "while_end", uniqueId++)
    endLabel = create_label(ss.str(), &current_context, ast, subScheme);


    expression cond;
    compile_cond_expression(cond, ast->getSubAst(ast_expression));

    if(!is_evaluable_type(cond.type)) {
        create_new_error(GENERIC, ast->line, ast->col, "while loop condition of type `" + type_to_str(cond.type) + "` must evaluate to true or false");
    }

    create_set_label_operation(subScheme, beginLabel);
    create_get_value_operation(subScheme, &cond.scheme, false, false);
    create_jump_if_false_operation(subScheme, endLabel);

    compile_block(ast->getSubAst(ast_block), subScheme, while_block, beginLabel, endLabel);
    current_context.blockInfo.reachable = !current_context.blockInfo.reachable;

//    if(!current_context.blockInfo.reachable && (cond.type != type_bool && !cond.type._bool) && (cond.type != type_integer && !cond.type.integer)) {
        current_context.blockInfo.reachable = true;
//    }

    create_jump_operation(subScheme, beginLabel);
    create_set_label_operation(subScheme, endLabel);

    add_scheme_operation(scheme, subScheme);
    delete subScheme;
}
