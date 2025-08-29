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
    sharp_type fieldType;
    uInt flags = flag_public;
    sharp_field *condField = NULL;

    expression cond;
    Ast *whenBlock = ast->getSubAst(ast_when_block);
    Token operand("==", SINGLE, ast->col, ast->line, EQEQ);

    controlPaths[WHEN_CONTROL_PATH] = true;
    if(ast->hasSubAst(ast_expression)) {
        compile_cond_expression(cond, ast->getSubAst(ast_expression));

        set_internal_variable_name(ss, "cond_var", uniqueId++)
        fieldType.copy(get_real_type(cond.type));
        condField = create_local_field(current_file, &current_context, ss.str(), flags, fieldType, normal_field, ast->getSubAst(ast_expression));

        APPLY_TEMP_SCHEME_WITH_TYPE(0, scheme_assign_value, *subScheme,
           operation_schema *condVariableScheme = new operation_schema();
           create_get_value_operation(&scheme_0, &cond.scheme, false, false);
           create_push_to_stack_operation(&scheme_0);

           create_local_field_access_operation(condVariableScheme, condField);
           create_get_value_operation(&scheme_0, condVariableScheme, false, false);
           create_pop_value_from_stack_operation(&scheme_0);
           create_unused_data_operation(&scheme_0);

           cond.type.copy(sharp_type(condField));
           cond.scheme.free();
           cond.scheme.copy(*condVariableScheme);
           delete condVariableScheme;
        )
    }

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
                    expression tmp, comparer, out;
                    tmp.type.copy(cond.type);
                    tmp.scheme.copy(cond.scheme);

                    compile_cond_expression(comparer, expressionAst);

                    if(ast->hasSubAst(ast_expression)) {
                        compile_binary_expression(&out, branch, tmp, comparer, operand);
                        extract_value_field_from_expression(out, "std#bool", expressionAst, true);
                        create_get_value_operation(whenClauseScheme, &out.scheme, false, false);
                    } else {
                        extract_value_field_from_expression(comparer, "std#bool", expressionAst, true);
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
