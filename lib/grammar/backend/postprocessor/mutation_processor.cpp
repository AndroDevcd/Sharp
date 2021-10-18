//
// Created by BNunnally on 9/19/2021.
//

#include "mutation_processor.h"
#include "../dependency/dependancy.h"
#include "../../taskdelegator/task_delegator.h"
#include "../access_flag.h"
#include "../types/sharp_class.h"
#include "../preprocessor/class_preprocessor.h"
#include "class_processor.h"
#include "../../compiler_info.h"

void process_mutation(Ast *ast) {
    sharp_type resolvedType =
            resolve(ast->getSubAst(ast_none)->getSubAst(ast_refrence_pointer),
                    resolve_hard_type | resolve_generic_type);

    if(resolvedType.type == type_class) {
        sharp_class *with_class = resolvedType._class;

        if(check_flag(with_class->flags, flag_stable)) {
            currThread->currTask->file->errors->createNewError(GENERIC, ast->line, ast->col,
                    "cannot mutate stable class `" + with_class->fullName + "`");
        }

        if(with_class->blueprintClass) {
            with_class->mutations.add(ast);
            process_generic_clone_mutations(with_class, ast);
        } else {
            process_mutation(with_class, ast);
        }
    } else {
        currThread->currTask->file->errors->createNewError(GENERIC, ast,
                "expected mutate host to be of type `class` but was found to be of type `" + type_to_str(resolvedType) + "`");
    }
}

void process_generic_clone_mutations(
        sharp_class *with_class,
        Ast *ast) {
    GUARD(globalLock)

    for(Int i = 0; i < with_class->genericClones.size(); i++) {
        process_mutation(with_class->genericClones.get(i), ast);
    }
}

void process_generic_mutations(
        sharp_class *with_class,
        sharp_class *blueprint) {
    GUARD(globalLock)

    for(Int i = 0; i < blueprint->mutations.size(); i++) {
        process_mutation(with_class, blueprint->mutations.get(i));
    }
}

void process_mutation(sharp_class *with_class, Ast *ast) {
    if(!with_class->mutations.find(ast)) {
        with_class->mutations.add(ast);
        pre_process_class(NULL, with_class, ast);
        process_class(NULL, with_class, ast);
    }
}
