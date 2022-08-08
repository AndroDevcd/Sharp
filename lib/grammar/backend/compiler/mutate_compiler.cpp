//
// Created by bknun on 7/3/2022.
//

#include "mutate_compiler.h"
#include "../types/types.h"
#include "compiler.h"
#include "../preprocessor/class_preprocessor.h"
#include "../postprocessor/class_processor.h"
#include "../astparser/ast_parser.h"

void compile_global_mutations(sharp_file *file) {
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
            }

            sharp_class *globalClass = resolve_class(currModule, global_class_name, false, false);
            create_context(globalClass, true);
            continue;
        }

        switch(trunk->getType()) {
            case ast_mutate_decl:
                compile_mutation(trunk);
                break;
            default:
                /* */
                break;
        }
    }
}

void compile_mutation(Ast* ast) {
    sharp_type resolvedType =
            resolve(ast->getSubAst(ast_name)->getSubAst(ast_refrence_pointer),
                    resolve_hard_type | resolve_generic_type);

    if(resolvedType.type == type_class) {
        sharp_class *with_class = resolvedType._class;

        if(with_class->blueprintClass) {
            GUARD(globalLock)

            for(Int i = 0; i < with_class->genericClones.size(); i++) {
                sharp_class *sc = with_class->genericClones.get(i);

                pre_process_class(NULL, sc, ast);
                process_class(NULL, sc, ast);
                compile_class(NULL, sc, ast);
            }
        } else {
            pre_process_class(NULL, with_class, ast);
            process_class(NULL, with_class, ast);
            compile_class(NULL, with_class, ast);
        }
    } else {
        currThread->currTask->file->errors->createNewError(GENERIC, ast,
                                                           "expected mutate host to be of type `class` but was found to be of type `" + type_to_str(resolvedType) + "`");
    }
}

