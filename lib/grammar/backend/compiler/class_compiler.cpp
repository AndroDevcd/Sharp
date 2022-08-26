//
// Created by bknun on 8/6/2022.
//

#include "class_compiler.h"
#include "../../sharp_file.h"
#include "../../compiler_info.h"
#include "../types/types.h"
#include "../astparser/ast_parser.h"
#include "enum_compiler.h"
#include "field_compiler.h"
#include "functions/function_compiler.h"
#include "functions/init_compiler.h"


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

    compile_class_fields(with_class, block);
    compile_class_mutations(with_class, block);
    compile_inits(with_class, block);
    compile_initialization_paring(with_class);
    compile_class_functions(with_class, block);
    compile_class_lambdas(with_class);


    for(Int i = 0; i < block->getSubAstCount(); i++) {
        Ast *trunk = block->getSubAst(i);

        switch(trunk->getType()) {
            case ast_generic_class_decl:
            case ast_class_decl:
                compile_class(with_class, NULL, trunk);
                break;
            case ast_enum_decl:
                compile_enum_fields(with_class, trunk);
                break;
            default:
                break;
        }
    }

    delete_context();
}

void compile_classes() {
    sharp_file *file = current_file;
    sharp_class *globalClass = NULL;

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

            globalClass = resolve_class(currModule, global_class_name, false, false);
            create_context(globalClass, true);
            continue;
        }

        switch(trunk->getType()) {
            case ast_class_decl:
                compile_class(globalClass, NULL, trunk);
                break;
            case ast_enum_decl:
                compile_enum_fields(globalClass, trunk);
                break;
            default:
                /* */
                break;
        }
    }

    delete_context();
}
