//
// Created by BNunnally on 9/9/2021.
//

#include "base_class_processor.h"
#include "../../sharp_file.h"
#include "../../taskdelegator/task_delegator.h"
#include "../astparser/ast_parser.h"
#include "../types/sharp_module.h"
#include "../types/sharp_class.h"

void process_base_class() {
    sharp_file *file = currThread->currTask->file;
    sharp_class *globalClass = NULL;

    for(Int i = 0; i < file->p->size(); i++)
    {
        if(panic) return;

        Ast *trunk = file->p->astAt(i);
        if(i == 0) {
            if(trunk->getType() == ast_module_decl) {
                string package = concat_tokens(trunk);
                currModule = get_module(package);
            } else {
                string package = "__$srt_undefined";
                currModule = get_module(package);
            }

            globalClass = resolve_class(currModule, global_class_name, false, false);
            continue;
        }

        switch(trunk->getType()) {
            case ast_enum_decl:
            case ast_class_decl:
            case ast_interface_decl:
                process_base_class(globalClass, NULL, trunk);
                break;
            default:
                break;
        }
    }
}

void process_base_class(
        sharp_class *parentClass,
        sharp_class *with_class,
        Ast *ast) {
    Ast* block = ast->getSubAst(ast_block), *trunk;
    bool isGeneric = ast->getType() == ast_generic_class_decl
            || ast->getType() == ast_generic_interface_decl;

    if(with_class == NULL) {
        string name = ast->getToken(0).getValue();
        with_class = resolve_class(parentClass, name, isGeneric, false);
    }

    create_context(parentClass, true);
    sharp_class *base = resolve_base_class(with_class, ast->getSubAst(ast_base_class));

    delete_context();
}

sharp_class* resolve_base_class(sharp_class *wit_class, Ast *ast) {
    if(ast != NULL) {
        Ast *refPtrAst = ast->getSubAst(ast_refrence_pointer);
        sharp_type type(resolve(refPtrAst));

        return NULL;
    } else {
        return NULL;
    }
}
