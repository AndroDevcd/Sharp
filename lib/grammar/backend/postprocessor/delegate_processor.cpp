//
// Created by BNunnally on 9/19/2021.
//

#include "delegate_processor.h"
#include "../types/sharp_class.h"
#include "function_processor.h"
#include "../../taskdelegator/task_delegator.h"
#include "../astparser/ast_parser.h"
#include "../../compiler_info.h"

void process_delegates() {
    sharp_file *file = currThread->currTask->file;
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
            case ast_generic_class_decl:
            case ast_class_decl:
                process_class_delegates(globalClass, NULL, trunk);
                break;
            default:
                /* ignore */
                break;
        }
    }

    delete_context();
}


void process_class_delegates(sharp_class* parentClass, sharp_class *with_class, Ast *ast) {
    Ast* block = ast->getSubAst(ast_block);

    if(with_class == NULL) {
        string name = ast->getToken(0).getValue();
        with_class = resolve_class(parentClass, name, ast->getType() == ast_generic_class_decl, false);
    }

    if(with_class->name == "std.io#atomic<std#int>") {
        int i = 0;
    }

    create_context(with_class, true);
    if(with_class->blueprintClass) {
        GUARD(globalLock)

        for(Int i = 0; i < with_class->genericClones.size(); i++) {
            process_class_delegates(NULL, with_class->genericClones.get(i), with_class->genericClones.get(i)->ast);
        }

        delete_context();
        return;
    } else
        process_delegates(with_class);

    for(Int i = 0; i < block->getSubAstCount(); i++) {
        Ast *trunk = block->getSubAst(i);

        switch(trunk->getType()) {
            case ast_generic_class_decl:
            case ast_class_decl:
                process_class_delegates(with_class, NULL, trunk);
                break;
            default:
                break;
        }
    }

    delete_context();
}

void process_delegates(sharp_class *with_class) {
    List<sharp_function*> functions, locatedFunctions, requiredFunctions;

    if(currThread->currTask->file->stage < pre_compilation_state)
        return;

    if(with_class->baseClass != NULL
        || !with_class->interfaces.empty()) {
        locate_functions_with_type(
                with_class, normal_function,
                false, locatedFunctions);

        if(with_class->baseClass != NULL) {
            locate_functions_with_type(
                    with_class->baseClass, delegate_function,
                    false, requiredFunctions);
        }

        for(Int i = 0; i < with_class->interfaces.size(); i++) {
            locate_functions_with_type(
                    with_class->interfaces.get(i), delegate_function,
                    true, requiredFunctions);
        }

        for(Int i = 0; i < locatedFunctions.size(); i++) {
            process_function_return_type(locatedFunctions.get(i));
        }

        for(Int i = 0; i < requiredFunctions.size(); i++) {
            process_function_return_type(requiredFunctions.get(i));
        }

        for(Int i = 0; i < locatedFunctions.size(); i++) {
            sharp_function *fun = locatedFunctions.get(i);

            if(check_flag(fun->flags, flag_override)) {
                functions.add(fun);
            }
        }

        locatedFunctions.free();
        bool foundFunc;
        for(Int i = 0; i < requiredFunctions.size(); i++) {
            sharp_function *delegate = requiredFunctions.get(i);
            foundFunc = false;

            for(Int j = 0; j < functions.size(); j++) {
                sharp_function *fun = functions.get(j);

                sharp_type comparer(delegate);
                sharp_type comparee(fun);
                if(fun->name == delegate->name && is_explicit_type_match(comparer, comparee)
                    && is_explicit_type_match(delegate->returnType, fun->returnType)) {
                    foundFunc = true;
                    fun->delegate = delegate;
                    create_dependency(fun, delegate);
                    create_dependency(delegate, fun);
                    delegate->impls.addif(fun);
                    break;
                }
            }

            if(!foundFunc) {
                stringstream err;
                err << "delegate function `" << function_to_str(delegate) << "` must be defined in class `"
                    << with_class->fullName << "`:";
                if(create_new_error(GENERIC, with_class->ast, err.str()) == 0)
                    print_impl_location(delegate->name, "delegate", delegate->implLocation);
            }
        }

        for(Int i = 0; i < functions.size(); i++) {
            if(functions.get(i)->delegate == NULL) {
                sharp_function *fun = functions.get(i);
                sharp_function *basefun = resolve_function(fun->name, fun->owner, fun->parameters,
                                                           delegate_function, exclude_all, fun->ast, true, false);

                if(basefun != NULL) {
                    fun->delegate = basefun;
                    create_dependency(fun, basefun);
                    create_dependency(basefun, fun);
                } else {
                    stringstream err;
                    err << "function `" << function_to_str(functions.get(i)) << "` overrides nothing in class `"
                        << with_class->fullName << "`:";
                    create_new_error(GENERIC, functions.get(i)->ast, err.str());
                }
            }
        }

        functions.free();
        requiredFunctions.free();
    }
}