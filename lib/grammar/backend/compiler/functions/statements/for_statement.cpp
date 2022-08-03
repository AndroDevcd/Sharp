//
// Created by bknun on 6/11/2022.
//

#include "for_statement.h"
#include "../../../../frontend/parser/Ast.h"
#include "../../../types/types.h"
#include "../function_compiler.h"
#include "../../expressions/expression.h"
#include "local_variable_decl.h"

void compile_for_statement_style_2(Ast *ast, operation_schema *scheme, bool *controlPaths) {
    stringstream ss;
    sharp_label *beginLabel;
    operation_schema *subScheme = new operation_schema();
    subScheme->schemeType = scheme_for;

    set_internal_label_name(ss, "for_begin", uniqueId++)
    beginLabel = create_label(ss.str(), &current_context, ast, subScheme);

    create_set_label_operation(subScheme, beginLabel);
    compile_block(ast->getSubAst(ast_block), subScheme, for_block);

    current_context.blockInfo.reachable = !current_context.blockInfo.reachable;
    create_jump_operation(subScheme, beginLabel);
    add_scheme_operation(scheme, subScheme);
}

void compile_for_statement(Ast *ast, operation_schema *scheme, bool *controlPaths) {
    if(ast->getType() == ast_for_style_2_statement)
        compile_for_statement_style_2(ast, scheme, controlPaths);
    else {
        stringstream ss;
        sharp_label *beginLabel, *endLabel;
        operation_schema *subScheme = new operation_schema();
        subScheme->schemeType = scheme_for;

        set_internal_label_name(ss, "for_begin", uniqueId++)
        beginLabel = create_label(ss.str(), &current_context, ast, subScheme);
        set_internal_label_name(ss, "for_end", uniqueId++)
        endLabel = create_label(ss.str(), &current_context, ast, subScheme);

        if(ast->hasSubAst(ast_variable_decl)) {
            compile_local_variable_statement(ast->getSubAst(ast_variable_decl), subScheme);
        }

        create_set_label_operation(subScheme, beginLabel);

        ALLOCATE_REGISTER_1X(0, subScheme,
            if(ast->hasSubAst(ast_for_expresion_cond)) {
                expression cond;
                compile_expression(cond, ast->getSubAst(ast_for_expresion_cond)->getSubAst(ast_expression));

                if(is_evaluable_type(cond.type)) {
                    create_get_value_operation(subScheme, &cond.scheme, false, false);
                    create_retain_numeric_value_operation(subScheme, register_0);
                    create_jump_if_false_operation(subScheme, endLabel);
                } else {
                    current_file->errors->createNewError(GENERIC, ast->line, ast->col, "for loop condition expression must be an evaluable type.");
                }
            }

            compile_block(ast->getSubAst(ast_block), subScheme, for_block, beginLabel, endLabel);
            current_context.blockInfo.reachable = true;


            if(ast->hasSubAst(ast_for_expresion_iter)) {
                expression iter;
                compile_expression(iter, ast->getSubAst(ast_for_expresion_iter)->getSubAst(ast_expression));
                create_get_value_operation(subScheme, &iter.scheme, false, false);
            }
            create_jump_operation(subScheme, beginLabel);
        )
        create_set_label_operation(subScheme, endLabel);

        add_scheme_operation(scheme, subScheme);
    }
}