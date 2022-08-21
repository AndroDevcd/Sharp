//
// Created by bknun on 7/2/2022.
//

#include "when_statement.h"
#include "../../../types/types.h"
#include "../../expressions/expression.h"
#include "../../compiler.h"
#include "../../expressions/binary/binary_expression.h"
#include "../function_compiler.h"

void compile_when_statement(Ast *ast, operation_schema *scheme, bool *controlPaths) {
    stringstream ss;
    string std = "std";
    sharp_label *nextClauseLabel, *clauseBeginLabel, *endLabel;
    operation_schema *subScheme = new operation_schema();
    subScheme->schemeType = scheme_when;

    expression cond;
    Ast *whenBlock = ast->getSubAst(ast_when_block);
    Token operand("==", SINGLE, ast->col, ast->line, EQEQ);

    controlPaths[WHEN_CONTROL_PATH] = true;
    if(ast->hasSubAst(ast_expression))
        compile_expression(cond, ast->getSubAst(ast_expression));

    set_internal_label_name(ss, "when_end", uniqueId++)
    endLabel = create_label(ss.str(), &current_context, ast, subScheme);

    for(Int i = 0; i < whenBlock->getSubAstCount(); i++) {
        Ast *branch = whenBlock->getSubAst(i);

        switch(branch->getType()) {
            case ast_when_clause: {
                set_internal_label_name(ss, "when_next_case", uniqueId++)
                nextClauseLabel = create_label(ss.str(), &current_context, ast, subScheme);
                set_internal_label_name(ss, "when_case_start", uniqueId++)
                clauseBeginLabel = create_label(ss.str(), &current_context, ast, subScheme);

                for(Int j = 0; j < branch->getSubAstCount() - 1; j++) {
                    Ast *expressionAst = branch->getSubAst(j);
                    expression comparer, out;

                    compile_expression(comparer, expressionAst);

                    if(ast->hasSubAst(ast_expression)) {
                        compile_binary_expression(&out, branch, cond, comparer, operand);
                        if(!is_evaluable_type(out.type)) {
                            current_file->errors->createNewError(GENERIC,  branch->line, branch->col, "when condition expression must evaluate to true or false");
                        }

                        create_get_value_operation(subScheme, &out.scheme, false, false);
                    } else {
                        if(!is_evaluable_type(comparer.type)) {
                            current_file->errors->createNewError(GENERIC,  branch->line, branch->col, "when condition expression must evaluate to true or false");
                        }

                        create_get_value_operation(subScheme, &comparer.scheme, false, false);
                    }


                    if((j + 1) < (branch->getSubAstCount() -1)) {
                        create_jump_if_true_operation(subScheme, clauseBeginLabel);
                    } else {
                        create_jump_if_false_operation(subScheme, nextClauseLabel);
                    }
                }

                create_set_label_operation(subScheme, clauseBeginLabel);
                if(!compile_block(branch->getSubAst(ast_block),subScheme,  when_block)) {
                    controlPaths[WHEN_CONTROL_PATH] = false;
                }

                if((i+1) < whenBlock->getSubAstCount()) {
                    create_jump_operation(subScheme, endLabel);
                }

                current_context.blockInfo.reachable = true;
                create_set_label_operation(subScheme, nextClauseLabel);
                break;
            }

            case ast_when_else_clause: {
                controlPaths[WHEN_ELSE_CONTROL_PATH] = compile_block(branch->getSubAst(ast_block), subScheme, when_block);
                break;
            }

            default:
                break;
        }
    }

    create_set_label_operation(subScheme, endLabel);
    create_no_operation(subScheme);
    add_scheme_operation(scheme, subScheme);
}