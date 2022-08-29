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
                operation_schema *whenClauseScheme = new operation_schema(scheme_when_clause);
                set_internal_label_name(ss, "when_next_case", uniqueId++)
                nextClauseLabel = create_label(ss.str(), &current_context, ast, whenClauseScheme);
                set_internal_label_name(ss, "when_case_start", uniqueId++)
                clauseBeginLabel = create_label(ss.str(), &current_context, ast, whenClauseScheme);

                for(Int j = 0; j < branch->getSubAstCount() - 1; j++) {
                    Ast *expressionAst = branch->getSubAst(j);
                    expression comparer, out;

                    compile_expression(comparer, expressionAst);

                    if(ast->hasSubAst(ast_expression)) {
                        recompile_expression(cond, ast->getSubAst(ast_expression));

                        compile_binary_expression(&out, branch, cond, comparer, operand);
                        if(!is_evaluable_type(out.type)) {
                            current_file->errors->createNewError(GENERIC,  branch->line, branch->col, "when condition expression must evaluate to true or false");
                        }

                        create_get_value_operation(whenClauseScheme, &out.scheme, false, false);
                    } else {
                        if(!is_evaluable_type(comparer.type)) {
                            current_file->errors->createNewError(GENERIC,  branch->line, branch->col, "when condition expression must evaluate to true or false");
                        }

                        create_get_value_operation(whenClauseScheme, &comparer.scheme, false, false);
                    }


                    if((j + 1) < (branch->getSubAstCount() -1)) {
                        create_jump_if_true_operation(whenClauseScheme, clauseBeginLabel);
                    } else {
                        create_jump_if_false_operation(whenClauseScheme, nextClauseLabel);
                    }
                }

                create_set_label_operation(whenClauseScheme, clauseBeginLabel);
                if(!compile_block(branch->getSubAst(ast_block),whenClauseScheme,  when_block)) {
                    controlPaths[WHEN_CONTROL_PATH] = false;
                }

                if((i+1) < whenBlock->getSubAstCount()) {
                    create_jump_operation(whenClauseScheme, endLabel);
                }

                current_context.blockInfo.reachable = true;
                create_set_label_operation(whenClauseScheme, nextClauseLabel);
                add_scheme_operation(subScheme, whenClauseScheme);

                delete whenClauseScheme;
                break;
            }

            case ast_when_else_clause: {
                operation_schema *whenElseClauseScheme = new operation_schema(scheme_when_else_clause);
                controlPaths[WHEN_ELSE_CONTROL_PATH] =
                        compile_block(branch->getSubAst(ast_block), whenElseClauseScheme, when_block);
                add_scheme_operation(subScheme, whenElseClauseScheme);

                delete whenElseClauseScheme;
                break;
            }

            default:
                break;
        }
    }

    create_set_label_operation(subScheme, endLabel);
    add_scheme_operation(scheme, subScheme);
    delete subScheme;
}
