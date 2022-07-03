//
// Created by bknun on 6/2/2022.
//

#include "compiler.h"
#include "../../sharp_file.h"
#include "../../compiler_info.h"
#include "../astparser/ast_parser.h"
#include "../types/sharp_module.h"
#include "../types/sharp_class.h"
#include "field_compiler.h"
#include "functions/init_compiler.h"

void compile_global_fields(sharp_class *with_class, sharp_file *file) {
    for(Int i = 0; i < file->p->size(); i++)
    {
        if(panic) return;

        Ast *trunk = file->p->astAt(i);
        switch(trunk->getType()) {
            case ast_variable_decl:
                compile_field(with_class, trunk);
                break;
            default:
                /* */
                break;
        }
    }
}

void __compile__() {
    sharp_file *file = current_file;
    sharp_class *globalClass = NULL;


    if(file->p->size() > 0) {
        Ast *trunk = file->p->astAt(0);
        if(trunk->getType() == ast_module_decl) {
            string package = concat_tokens(trunk);
            currModule = create_module(package);
        } else {
            string package = "__$srt_undefined";
            currModule = create_module(package);

            currThread->currTask->file->errors->createNewError(GENERIC, trunk->line, trunk->col, "module declaration must be ""first in every file");
        }

        globalClass = resolve_class(currModule, global_class_name, false, false);
        create_context(globalClass, true);
    }

    compile_global_fields(globalClass, file);
    for(Int i = 0; i < file->p->size(); i++)
    {
        if(panic) return;

        Ast *trunk = file->p->astAt(i);
        if(i == 0) {
            if(trunk->getType() == ast_module_decl) {
                string package = concat_tokens(trunk);
                currModule = create_module(package);
            } else {
                string package = "__$srt_undefined";
                currModule = create_module(package);

                currThread->currTask->file->errors->createNewError(GENERIC, trunk->line, trunk->col, "module declaration must be ""first in every file");
            }

            globalClass = resolve_class(currModule, global_class_name, false, false);
            create_context(globalClass, true);
            continue;
        }

        switch(trunk->getType()) {
            case ast_class_decl:
                compile_class(globalClass, NULL, trunk);
                break;
            default:
                /* */
                break;
        }
    }

    delete_context();
}

void compile_class(sharp_class* parentClass, sharp_class *with_class, Ast *ast) {
    Ast* block = ast->getSubAst(ast_block);

    if(with_class == NULL) {
        string name = ast->getToken(0).getValue();
        with_class = resolve_class(parentClass, name, false, false);
    }

    create_context(with_class, true);
    if(with_class->blueprintClass) {
        GUARD(globalLock)

        for(Int i = 0; i < with_class->genericClones.size(); i++) {
            compile_class(NULL, with_class->genericClones.get(i), with_class->genericClones.get(i)->ast);
        }

        delete_context();
        return;
    }

    compile_class_fields(with_class);
    compile_inits(with_class);
    // todo: compile all other functions
    // todo: compile uncompiled functions like field setters and getters

    for(Int i = 0; i < block->getSubAstCount(); i++) {
        Ast *trunk = block->getSubAst(i);

        switch(trunk->getType()) {
            case ast_generic_class_decl:
            case ast_class_decl:
                compile_class(with_class, NULL, trunk);
                break;
            default:
                break;
        }
    }

    delete_context();
}

/**
 * This function ensures that all control paths return a value before leaving a block.
 * @param controlPaths
 * @return
 */
bool validate_control_paths(bool *controlPaths) {
    return controlPaths[MAIN_CONTROL_PATH]
           || (controlPaths[IF_CONTROL_PATH] && controlPaths[ELSEIF_CONTROL_PATH] && controlPaths[ELSE_CONTROL_PATH])
           || (controlPaths[TRY_CONTROL_PATH] && controlPaths[CATCH_CONTROL_PATH])
           || (controlPaths[WHEN_CONTROL_PATH] && controlPaths[WHEN_ELSE_CONTROL_PATH]);
}
