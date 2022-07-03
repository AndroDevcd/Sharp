//
// Created by bknun on 6/2/2022.
//

#include "function_compiler.h"
#include "../../context/context.h"
#include "../../../taskdelegator/task_delegator.h"
#include "../../types/types.h"
#include "../../../compiler_info.h"
#include "../compiler.h"
#include "statements/statement_compiler.h"

void compile_function(sharp_function *function) {
    create_context(&current_context, function, check_flag(function->flags, flag_static));
    if(function->scheme == NULL)
        function->scheme = new operation_schema();
    bool codePathsReturnValue;

    if(function->type == initializer_function) {
        codePathsReturnValue = compile_block(function->ast->getSubAst(ast_block), function->scheme);
    } else {
        // todo: support the rest here
    }


    if(!codePathsReturnValue && function->returnType.type != type_nil) {
        current_file->errors->createNewError(GENERIC, function->ast->getSubAst(ast_block), "not all code paths return a value");
    }
    delete_context();
}

bool compile_block(
        Ast *ast,
        operation_schema *scheme,
        block_type bt,
        sharp_label *beginLabel,
        sharp_label *endLabel,
        operation_schema *lockScheme,
        sharp_label *finallyLabel) {
    create_block(&current_context, bt);
    current_context.blockInfo.beginLabel = beginLabel;
    current_context.blockInfo.endLabel = endLabel;
    current_context.blockInfo.lockScheme = lockScheme;
    current_context.blockInfo.finallyLabel = finallyLabel;

    bool controlPaths[]
         = {
                 false, // MAIN_CONTROL_PATH
                 false, // IF_CONTROL_PATH
                 false, // ELSEIF_CONTROL_PATH
                 false,  // ELSE_CONTROL_PATH
                 false,  // TRY_CONTROL_PATH
                 false,  // CATCH_CONTROL_PATH
                 false,  // WHEN_CONTROL_PATH
                 false   // WHEN_ELSE_CONTROL_PATH
         };

    for(unsigned int i = 0; i < ast->getSubAstCount(); i++) {
        Ast *branch = ast->getSubAst(i);

        if(branch->getType() == ast_block) {
            compile_block(branch, scheme);
            continue;
        } else {
            branch = branch->getSubAst(0);
            compile_statement(branch, scheme, controlPaths);
        }
    }

    delete_block();
    return validate_control_paths(controlPaths);
}