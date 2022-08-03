//
// Created by bknun on 7/3/2022.
//

#include "mutate_compiler.h"
#include "../types/types.h"
#include "compiler.h"
#include "../preprocessor/class_preprocessor.h"
#include "../postprocessor/class_processor.h"

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

