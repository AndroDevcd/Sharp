//
// Created by bknun on 8/8/2022.
//

#include "dependency_runner.h"
#include "../../../compiler_info.h"
#include "../../types/types.h"
#include "optimizer.h"
#include "../generation/generator.h"

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
        if(fun->fullName == "platform#build.static_init<build>") {
            int i = 0;
        }
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

int require_module(sharp_module *mod, string name) {
    if(mod == NULL) {
        cout << PROG_NAME << ": fatal error: missing required module dependency `" << name << "`, this module may be obfuscated or not present." << endl;
        return 1;
    } else {
        if(mod->obfuscateModifier > modifier_keep_inclusive) {
            cout << PROG_NAME << ": fatal error: missing required module dependency `" << name << "`, this module may be obfuscated." << endl;
            return 1;
        }
    }

    return 0;
}

int require_class(string fullName) {
    for(Int i = 0; i < compressedCompilationClasses.size(); i++) {
        auto klazz = compressedCompilationClasses.get(i);
        if(klazz->fullName == fullName) {
            if(klazz->obfuscateModifier <= modifier_keep_inclusive) {
                return 0;
            } else break;
        }
    }

    cout << PROG_NAME << ": fatal error: missing required class dependency `" << fullName << "`, this class may be obfuscated or missing." << endl;
    return 1;
}

void validate_required_dependencies() {

    // validate modules
    int error = 0;
    string mod;
    mod = "std";
    error += require_module(get_module(mod), "std");
    mod = "std.io";
    error += require_module(get_module(mod), "std.io");
    mod = "std.io.fiber";
    error += require_module(get_module(mod), "std.io.fiber");
    mod = "platform.kernel";
    error += require_module(get_module(mod), "platform.kernel");
    

    // validate classes
    error += require_class("std#throwable");
    error += require_class("std#exception");
    error += require_class("std#error");
    error += require_class("std#_enum_");
    error += require_class("std#illegal_argument_exception");
    error += require_class("std#illegal_state_exception");
    error += require_class("std#out_of_bounds_exception");
    error += require_class("std#invalid_operation_exception");
    error += require_class("std#nullptr_exception");
    error += require_class("std#runtime_exception");
    error += require_class("std#stack_overflow_exception");
    error += require_class("std#thread_stack_exception");
    error += require_class("std#class_cast_exception");
    error += require_class("std#out_of_memory_exception");
    error += require_class("std#invalid_operation_exception");
    error += require_class("std#incompatible_class_exception");
    error += require_class("std#unsatisfied_link_error");
    error += require_class("std#object_import_error");
    error += require_class("platform.kernel#stack_state");
    error += require_class("std.io#thread");
    error += require_class("std#string");
    error += require_class("std#char_array");
    error += require_class("std#int");
    error += require_class("std#number<_int32, std#int>");
    error += require_class("std.io.fiber#fiber");

    if(error > 0) {
        exit(1);
    }
}

