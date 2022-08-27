//
// Created by bknun on 8/8/2022.
//

#include "dependency_runner.h"
#include "../../../compiler_info.h"
#include "../../types/types.h"
#include "optimizer.h"

void run_and_mark_tree(List<dependency> &dependencies);

void mark(sharp_field *sf);
void mark(sharp_function *fun);
void mark(sharp_class *sc);
void mark(sharp_file *sf);

void mark(sharp_field *sf) {
    if(!sf->used) {
        sf->used = true;
        run_and_mark_tree(sf->dependencies);
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
    }
}

void mark(sharp_class *sc) {
    if(!sc->used) {
        sc->used = true;
        run_and_mark_tree(sc->dependencies);
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
    mark(genesis_method->implLocation.file);
    mark(user_main_method);
    mark(user_main_method->implLocation.file);
}

