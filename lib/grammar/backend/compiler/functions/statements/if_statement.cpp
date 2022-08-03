//
// Created by bknun on 6/4/2022.
//

#include "if_statement.h"
#include "../../expressions/expression.h"
#include "../../../../compiler_info.h"
#include "../../../types/types.h"
#include "../function_compiler.h"
#include "../../compiler.h"

void compile_if_statement(Ast *ast, operation_schema *scheme, bool *controlPaths) {
    expression cond;
    stringstream ss;
    sharp_label *endLabel, *blockEndLabel;
    operation_schema *subScheme = new operation_schema();
    subScheme->schemeType = scheme_if;

    compile_expression(cond, ast->getSubAst(ast_expression));
    if(!is_evaluable_type(cond.type)) {
        current_file->errors->createNewError(GENERIC, ast->line, ast->col, "if statement condition expression must evaluate to true or false");
    }

    set_internal_label_name(ss, "if_end", uniqueId++)
    endLabel = create_label(ss.str(), &current_context, ast, subScheme);
    create_get_value_operation(subScheme, &cond.scheme, false, false);

    if(ast->getSubAstCount() > 2) {
        set_internal_label_name(ss, "if_block_end", uniqueId++)
        blockEndLabel = create_label(ss.str(), &current_context, ast, subScheme);
        create_jump_if_false_operation(subScheme, blockEndLabel);

        controlPaths[IF_CONTROL_PATH] = compile_block(ast->getSubAst(ast_block), subScheme, if_block);
        controlPaths[ELSEIF_CONTROL_PATH] = true;
        current_context.blockInfo.reachable = true;
        create_jump_operation(subScheme, endLabel);

        create_set_label_operation(subScheme, blockEndLabel);

        for(Int i = 2; i < ast->getSubAstCount(); i++) {
            Ast *branch = ast->getSubAst(i);

            switch(branch->getType()) {
                case ast_elseif_statement: {
                    APPLY_TEMP_SCHEME_WITH_TYPE(0, scheme_elseif, *subScheme,
                        expression elseIfCond;
                        compile_expression(elseIfCond, ast->getSubAst(ast_expression));
                        if(!is_evaluable_type(elseIfCond.type)) {
                            current_file->errors->createNewError(GENERIC, ast->line, ast->col, "else if statement condition expression must evaluate to true or false");
                        }

                        set_internal_label_name(ss, "if_block_end", uniqueId++)
                        blockEndLabel = create_label(ss.str(), &current_context, ast, &scheme_0);
                        create_get_value_operation(&scheme_0, &cond.scheme, false, false);
                        create_jump_if_false_operation(&scheme_0, blockEndLabel);

                        if(!compile_block(branch->getSubAst(ast_block), &scheme_0, elseif_block)) {
                            controlPaths[ELSEIF_CONTROL_PATH] = false;
                        }

                        if((i+1) < ast->getSubAstCount()) {
                            create_jump_operation(&scheme_0, endLabel);
                        }

                        create_set_label_operation(&scheme_0, blockEndLabel);
                    )
                    break;
                }

                case ast_else_statement: {
                    APPLY_TEMP_SCHEME_WITH_TYPE(0, scheme_else, *subScheme,
                        controlPaths[ELSE_CONTROL_PATH] = compile_block(branch->getSubAst(ast_block), &scheme_0, else_block);
                    )
                    break;
                }
                default: {
                    current_file->errors->createNewError(INTERNAL_ERROR, ast->line, ast->col, "unexpected AST in if statement");
                    break;
                }
            }

            current_context.blockInfo.reachable = true;
        }
    } else {
        create_jump_if_false_operation(subScheme, endLabel);
        compile_block(ast->getSubAst(ast_block), subScheme, if_block);
        current_context.blockInfo.reachable = true;
    }

    create_set_label_operation(subScheme, endLabel);
    add_scheme_operation(scheme, subScheme);
}
