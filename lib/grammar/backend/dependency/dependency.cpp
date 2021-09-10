//
// Created by BNunnally on 8/31/2021.
//

#include "dependancy.h"
#include "../../compiler_info.h"
#include "../types/sharp_module.h"
#include "../types/sharp_class.h"
#include "../../sharp_file.h"
#include "../types/sharp_function.h"
#include "../types/sharp_field.h"
#include "../../taskdelegator/task_delegator.h"
#include "../types/sharp_alias.h"
#include "../types/import_group.h"


sharp_class* resolve_class(
        sharp_module* module,
        string name,
        bool isGeneric,
        bool matchName) {
    sharp_class *sc = NULL;
    GUARD(module->moduleLock)
    List<sharp_class*> *searchList = &module->classes;

    if(isGeneric) searchList = &module->genericClasses;
    for(Int i = 0; i < searchList->size(); i++) {
        sc = searchList->get(i);

        if((matchName && sc->name.find(name) != string::npos)
            || sc->name == name) {
            return sc;
        }
    }

    return NULL;
}


sharp_class* resolve_class(
        sharp_class* sc,
        string name,
        bool isGeneric,
        bool matchName) {
    sharp_class *child = NULL;
    GUARD(sc->mut)
    List<sharp_class*> *searchList = &sc->children;

    if(isGeneric) searchList = &sc->generics;
    for(Int i = 0; i < searchList->size(); i++) {
        sc = searchList->get(i);

        if((matchName && sc->name.find(name) != string::npos)
           || sc->name == name) {
            return sc;
        }
    }

    return NULL;
}

sharp_class* resolve_class(
                import_group* group,
                string name,
                bool isGeneric,
                bool matchName) {
    List<sharp_module*> &imports = group->modules;
    sharp_class *sc;

    for(Int i = 0; i < imports.size(); i++) {
        if((sc = resolve_class(
                imports.get(i),
                name,
                isGeneric,
                matchName)) != NULL) {
            return sc;
        }
    }

    return NULL;
}

import_group* resolve_import_group(sharp_file *file, string name) {
    List<import_group*> &groups = file->importGroups;

    for(Int i = 0; i < groups.size(); i++) {
        if(groups.get(i)->name == name) {
            return groups.get(i);
        }
    }

    return NULL;
}
sharp_class* resolve_class(
        sharp_file* file,
        string name,
        bool isGeneric,
        bool matchName) {
    List<sharp_module*> &imports = file->imports;
    sharp_class *sc;

    for(Int i = 0; i < imports.size(); i++) {
        if((sc = resolve_class(
                imports.get(i),
                name,
                isGeneric,
                matchName)) != NULL) {
            return sc;
        }
    }

    return NULL;
}


sharp_field* resolve_field(string name, sharp_module *module) {
    sharp_class *sc = NULL;
    sharp_field *sf = NULL;
    GUARD(module->moduleLock)
    List<sharp_class*> *searchList = &module->classes;

    for(Int i = 0; i < searchList->size(); i++) {
        sc = searchList->get(i);

        if(check_flag(sc->flags, flag_global)
            && (sf = resolve_field(name, sc)) != NULL) {
            return sf;
        }
    }

    return NULL;
}

sharp_field* resolve_field(string name, import_group *group) {
    List<sharp_module*> &imports = group->modules;
    sharp_field *sf;

    for(Int i = 0; i < imports.size(); i++) {
        if((sf = resolve_field(name, imports.get(i))) != NULL) {
            return sf;
        }
    }

    return NULL;
}

sharp_field* resolve_field(string name, sharp_file *file) {
    List<sharp_module*> &imports = file->imports;
    sharp_field *sf;

    for(Int i = 0; i < imports.size(); i++) {
        if((sf = resolve_field(name, imports.get(i))) != NULL) {
            return sf;
        }
    }

    return NULL;
}

sharp_field* resolve_field(string name, sharp_class *searchClass) {
    for(Int i = 0; i < searchClass->fields.size(); i++) {
        if(searchClass->fields.get(i)->name == name) {
            return searchClass->fields.get(i);
        }
    }

    return NULL;
}


sharp_alias* resolve_alias(string name, sharp_module *module) {
    sharp_class *sc = NULL;
    sharp_alias *sa = NULL;
    GUARD(module->moduleLock)
    List<sharp_class*> *searchList = &module->classes;

    for(Int i = 0; i < searchList->size(); i++) {
        sc = searchList->get(i);

        if(check_flag(sc->flags, flag_global)
            && (sa = resolve_alias(name, sc)) != NULL) {
            return sa;
        }
    }

    return NULL;
}

sharp_alias* resolve_alias(string name, import_group *group) {
    List<sharp_module*> &imports = group->modules;
    sharp_alias *sa;

    for(Int i = 0; i < imports.size(); i++) {
        if((sa = resolve_alias(name, imports.get(i))) != NULL) {
            return sa;
        }
    }

    return NULL;
}

sharp_alias* resolve_alias(string name, sharp_file *file) {
    List<sharp_module*> &imports = file->imports;
    sharp_alias *sa;

    for(Int i = 0; i < imports.size(); i++) {
        if((sa = resolve_alias(name, imports.get(i))) != NULL) {
            return sa;
        }
    }

    return NULL;
}

sharp_alias* resolve_alias(string name, sharp_class *searchClass) {
    for(Int i = 0; i < searchClass->aliases.size(); i++) {
        if(searchClass->aliases.get(i)->name == name) {
            return searchClass->aliases.get(i);
        }
    }

    return NULL;
}

sharp_function* resolve_function(
        string name,
        import_group *group,
        List<sharp_field*> &parameters,
        Int functionType,
        uInt excludeMatches,
        Ast *resolveLocation,
        bool checkBaseClass,
        bool implicitCheck) {
    List<sharp_module*> &imports = group->modules;
    sharp_function *sf;

    for(Int i = 0; i < imports.size(); i++) {
        if((sf = resolve_function(
                name, imports.get(i), parameters,
                functionType, excludeMatches,
                resolveLocation, checkBaseClass,
                implicitCheck)) != NULL) {
            return sf;
        }
    }

    return NULL;
}

sharp_function* resolve_function(
        string name,
        sharp_file *file,
        List<sharp_field*> &parameters,
        Int functionType,
        uInt excludeMatches,
        Ast *resolveLocation,
        bool checkBaseClass,
        bool implicitCheck) {
    List<sharp_module*> &imports = file->imports;
    sharp_function *sf;

    for(Int i = 0; i < imports.size(); i++) {
        if((sf = resolve_function(
                name, imports.get(i), parameters,
                functionType, excludeMatches,
                resolveLocation, checkBaseClass,
                implicitCheck)) != NULL) {
            return sf;
        }
    }

    return NULL;
}

sharp_function* resolve_function(
        string name,
        sharp_module *module,
        List<sharp_field*> &parameters,
        Int functionType,
        uInt excludeMatches,
        Ast *resolveLocation,
        bool checkBaseClass,
        bool implicitCheck) {
    sharp_class *sc = NULL;
    sharp_function *sf = NULL;
    GUARD(module->moduleLock)
    List<sharp_class*> *searchList = &module->classes;

    for(Int i = 0; i < searchList->size(); i++) {
        sc = searchList->get(i);

        if(check_flag(sc->flags, flag_global)
        && (sf = resolve_function(
                name, sc, parameters,
                functionType, excludeMatches,
                resolveLocation, checkBaseClass,
                implicitCheck)) != NULL) {
            return sf;
        }
    }

    return NULL;
}


sharp_function* resolve_function(
        string name,
        sharp_class *searchClass,
        List<sharp_field*> &parameters,
        Int functionType,
        uInt excludedMatches,
        Ast *resolveLocation,
        bool checkBaseClass,
        bool implicitCheck) {
    List<sharp_function*> locatedFunctions;
    sharp_function *resolvedFunction = NULL;
    bool ambiguous = false;
    sharp_function mock_function("mock", searchClass, impl_location(),
            flag_none, NULL, parameters, sharp_type(), undefined_function);

    locate_functions_with_name(name, searchClass, functionType, checkBaseClass,
            locatedFunctions);

    if(!locatedFunctions.empty()) {
        for(Int i = 0; i < locatedFunctions.size(); i++) {
            sharp_function *func = locatedFunctions.get(i);
            mock_function.returnType = func->returnType;

            if(is_explicit_type_match(sharp_type(func), sharp_type(&mock_function))) {
                if(resolvedFunction == NULL) resolvedFunction = func;
                else ambiguous = true;
            }
        }

        if(implicitCheck) {

            for(Int i = 0; i < locatedFunctions.size(); i++) {
                sharp_function *func = locatedFunctions.get(i);
                mock_function.returnType = func->returnType;

                if(is_implicit_type_match(
                        sharp_type(func),
                        sharp_type(&mock_function),
                        excludedMatches)) {
                    if(resolvedFunction == NULL) resolvedFunction = func;
                    else ambiguous = true;
                }
            }
        }
    }

    if(ambiguous && resolveLocation != NULL) {
        currThread->currTask->file->errors->createNewError(GENERIC, resolveLocation->line, resolveLocation->col, "call to method `" + name + "` is ambiguous");
    }

    return resolvedFunction;
}

sharp_class* resolve_class(string name, bool isGeneric, bool matchName) {
    GUARD(globalLock)

    sharp_class *sc = NULL;
    for(Int i = 0; i < modules.size(); i++) {
        if((sc = resolve_class(modules.get(i), name, isGeneric, matchName)) != NULL) {
            return sc;
        }
    }

    return NULL;
}

sharp_type resolve(Ast *ast) {
    sharp_type type;
    if(ast->getType() == ast_refrence_pointer) {

    }

    return type;
}

void create_dependency(sharp_class* depender, sharp_class* dependee) {
    if(depender != dependee)
        depender->dependencies.addif(dependency(dependee));
    create_dependency(depender->implLocation.file, dependee->implLocation.file);
}

void create_dependency(sharp_file* depender, sharp_file* dependee) {
    if(depender != dependee)
        depender->dependencies.addif(dependency(dependee));
}

void create_dependency(sharp_function* depender, sharp_function* dependee) {
    if(depender != dependee)
        depender->dependencies.addif(dependency(dependee));
    create_dependency(depender->owner, dependee->owner);
}

void create_dependency(sharp_function* depender, sharp_class* dependee) {
    depender->dependencies.addif(dependency(dependee));
    create_dependency(depender->owner, dependee);
}

void create_dependency(sharp_function* depender, sharp_field* dependee) {
    depender->dependencies.addif(dependency(dependee));
    create_dependency(depender->owner, dependee->owner);
}

void create_dependency(sharp_field* depender, sharp_function* dependee) {
    depender->dependencies.addif(dependency(dependee));
    create_dependency(depender->owner, dependee->owner);
}

void create_dependency(sharp_field* depender, sharp_class* dependee) {
    depender->dependencies.addif(dependency(dependee));
    create_dependency(depender->owner, dependee);
}

void create_dependency(sharp_field* depender, sharp_field* dependee) {
    if(depender != dependee)
        depender->dependencies.addif(dependency(dependee));
    create_dependency(depender->owner, dependee->owner);
}

