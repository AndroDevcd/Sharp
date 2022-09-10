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
    subScheme->schemeType = scheme_for_infinite;

    set_internal_label_name(ss, "for_begin", uniqueId++)
    beginLabel = create_label(ss.str(), &current_context, ast, subScheme);

    create_set_label_operation(subScheme, beginLabel);
    compile_block(ast->getSubAst(ast_block), subScheme, for_block);

    current_context.blockInfo.reachable = !current_context.blockInfo.reachable;
    create_jump_operation(subScheme, beginLabel);
    add_scheme_operation(scheme, subScheme);
    delete subScheme;
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

        create_block(&current_context, normal_block);
        if(ast->hasSubAst(ast_variable_decl)) {
            operation_schema *varScheme = new operation_schema(scheme_master);
            compile_local_variable_statement(ast->getSubAst(ast_variable_decl), varScheme);
            add_scheme_operation(subScheme, varScheme);

            delete varScheme;
        }

        create_set_label_operation(subScheme, beginLabel);

        if(ast->hasSubAst(ast_for_expresion_cond)) {
            expression cond;
            compile_cond_expression(cond, ast->getSubAst(ast_for_expresion_cond)->getSubAst(ast_expression));

            if(is_evaluable_type(cond.type)) {
                operation_schema *condScheme = new operation_schema(scheme_for_cond);
                create_get_value_operation(condScheme, &cond.scheme, false, false);
                create_jump_if_false_operation(condScheme, endLabel);
                add_scheme_operation(subScheme, condScheme);

                delete condScheme;
            } else {
                create_new_error(GENERIC, ast->line, ast->col, "for loop condition expression must be an evaluable type.");
            }
        }

        compile_block(ast->getSubAst(ast_block), subScheme, for_block, beginLabel, endLabel);
        current_context.blockInfo.reachable = true;


        if(ast->hasSubAst(ast_for_expresion_iter)) {
            expression iter;
            operation_schema *iterScheme = new operation_schema(scheme_for_iter);

            compile_expression(iter, ast->getSubAst(ast_for_expresion_iter)->getSubAst(ast_expression));
            create_get_value_operation(iterScheme, &iter.scheme, false, false);

            add_scheme_operation(subScheme, iterScheme);

            delete iterScheme;
        }
        
        delete_block();
        create_jump_operation(subScheme, beginLabel);
        create_set_label_operation(subScheme, endLabel);

        add_scheme_operation(scheme, subScheme);
        delete subScheme;
    }
}