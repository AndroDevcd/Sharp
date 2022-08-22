//
// Created by bknun on 6/2/2022.
//

#include "statement_compiler.h"
#include "../../../../compiler_info.h"
#include "../../../types/types.h"
#include "../../../../settings/settings.h"
#include "return_statement.h"
#include "if_statement.h"
#include "../../expressions/expression.h"
#include "local_variable_decl.h"
#include "local_alias_statement.h"
#include "for_statement.h"
#include "for_each_statement.h"
#include "do_while_statement.h"
#include "while_statement.h"
#include "throw_statement.h"
#include "continue_statement.h"
#include "lock_statement.h"
#include "try_catch_statement.h"
#include "break_statement.h"
#include "when_statement.h"


void compile_statement(Ast *ast, operation_schema *scheme, bool *controlPaths) {
    if(current_context.blockInfo.line == -1 || current_context.blockInfo.line < ast->line) {
        current_context.blockInfo.line = ast->line;
        APPLY_TEMP_SCHEME(1, *current_context.functionCxt->scheme,
              create_line_record_operation(&scheme_1, ast->line);
        )
    }

    if(!current_context.blockInfo.reachable) {
        if(ast->getType() != ast_label_decl)
            create_new_warning(GENERIC, __w_general, ast, "unreachable statement");
        current_context.blockInfo.reachable = true;
    }

    switch(ast->getType()) {
        case ast_return_stmnt:
            compile_return_statement(ast, scheme, controlPaths);
            break;
        case ast_if_statement:
            compile_if_statement(ast, scheme, controlPaths);
            break;
        case ast_expression: {
            expression e;
            compile_expression(e, ast);

            APPLY_TEMP_SCHEME_WITH_TYPE(0, scheme_none, *scheme,
                scheme_0.copy(e.scheme);
                create_unused_expression_data_operation(&scheme_0);
            )
            break;
        }
        case ast_variable_decl: {
            compile_local_variable_statement(ast, scheme);

            APPLY_TEMP_SCHEME_WITH_TYPE(0, scheme_unused_data, *scheme,
                  create_unused_expression_data_operation(&scheme_0);
            )
            break;
        }
        case ast_alias_decl: {
            compile_local_alias_statement(ast, scheme);
            break;
        }
        case ast_for_statement:
        case ast_for_style_2_statement:  {
            compile_for_statement(ast, scheme, controlPaths);
            break;
        }
        case ast_foreach_statement: {
            compile_for_each_statement(ast, scheme, controlPaths);
            break;
        }
        case ast_while_statement: {
            compile_while_statement(ast, scheme, controlPaths);
            break;
        }
        case ast_do_while_statement: {
            compile_do_while_statement(ast, scheme, controlPaths);
            break;
        }
        case ast_throw_statement: {
            compile_throw_statement(ast, scheme, controlPaths);
            break;
        }
        case ast_continue_statement: {
            compile_continue_statement(ast, scheme);
            break;
        }
        case ast_break_statement: {
            compile_break_statement(ast, scheme);
            break;
        }
        case ast_lock_statement: {
            compile_lock_statement(ast, scheme, controlPaths);
            break;
        }
        case ast_trycatch_statement: {
            compile_try_catch_statement(ast, scheme, controlPaths);
            break;
        }
        case ast_when_statement: {
            compile_when_statement(ast, scheme, controlPaths);
            break;
        }
        case ast_assembly_statement: {
            // todo: impl
            break;
        }
        default:
            stringstream err;
            err << ": unknown ast type: " << ast->getType();
            currThread->currTask->file->errors->createNewError(INTERNAL_ERROR, ast->line, ast->col, err.str());
            break;
    }
}
