//
// Created by bknun on 8/8/2022.
//

#include "dependency_runner.h"
#include "../../../compiler_info.h"
#include "../../types/types.h"
#include "optimizer.h"

void run_and_mark_tree(List<dependency> &dependencies);

void mark(sharp_field *sf) {
    if(!sf->used) {
        sf->used = true;
        run_and_mark_tree(sf->dependencies);
        mark(sf->implLocation.file);
    }
}

void mark(sharp_function *fun) {
    if(!fun->used) {
        fun->used = true;
        for(Int i = 0; i < fun->locals.size(); i++) {
            mark(fun->locals.get(i));
        }

        if(fun->type == delegate_function) {
            for(Int i = 0; i < fun->impls.size(); i++) {
                mark(fun->impls.get(i));
                mark(fun->impls.get(i)->implLocation.file);
                mark(fun->impls.get(i)->owner);
            }
        }

        run_and_mark_tree(fun->dependencies);
        mark(fun->implLocation.file);
    }
}

void mark(sharp_class *sc) {
    if(!sc->used) {
        sc->used = true;
        if(sc->type == class_enum) {
            for(Int i = 0; i < sc->fields.size(); i++) {
                mark(sc->fields.get(i));
            }

            for(Int i = 0; i < sc->functions.size(); i++) {
                mark(sc->functions.get(i));
            }
        }

        run_and_mark_tree(sc->dependencies);
        mark(sc->implLocation.file);
    }
}

void mark(sharp_file *sf) {
    if(!sf->used) {
        sf->used = true;
        compressedCompilationFiles.addif(sf);
        run_and_mark_tree(sf->dependencies);
    }
}

void run_and_mark_tree(List<dependency> &dependencies) {
    for(Int i = 0; i < dependencies.size(); i++) {
        dependency *d = &dependencies.get(i);

        switch(d->type) {
            case no_dependency: {
                // shouldn't happen
                break;
            }
            case dependency_file: {
                mark(d->fileDependency);
                break;
            }
            case dependency_class: {
                mark(d->classDependency);
                break;
            }
            case dependency_function: {
                mark(d->functionDependency);
                break;
            }
            case dependency_field: {
                mark(d->fieldDependency);
                break;
            }
        }
    }
}

void markRelevantDependencyMembers() {
    mark(genesis_method);
    mark(user_main_method);
    mark(static_init_method);
}

