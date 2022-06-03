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
    create_context(&curr_context, function, check_flag(function->flags, flag_static));

    if(function->type == initializer_function) {
        compile_block(function->ast->getSubAst(ast_block));
    } else {
        // todo: support the rest here
    }

    delete_context();
}

bool compile_block(Ast *ast) {
    curr_context.blockId++;
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
            compile_block(branch);
            continue;
        } else {
            branch = branch->getSubAst(0);
            compile_statement(branch, controlPaths);
        }
    }

    curr_context.blockId--;
    return validate_control_paths(controlPaths)
}