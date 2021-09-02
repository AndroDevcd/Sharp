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


sharp_function* resolve_function(
        string name,
        sharp_class *searchClass,
        List<sharp_type> parameters,
        Int functionType,
        Ast *resolveLocation,
        bool checkBaseClass,
        bool initializerCheck) {
    List<sharp_function*> locatedFunctions;
    List<sharp_function*> matchedFunctions;
    sharp_function *resolvedFunction = NULL;

    locate_functions_with_name(name, searchClass, functionType, checkBaseClass,
            locatedFunctions);

    if(!locatedFunctions.empty()) {
        for(Int i = 0; i < locatedFunctions.size(); i++) {
            if(is_fully_qualified_function(locatedFunctions.get(i)))
        }
    }

    return resolvedFunction;
}

sharp_class* resolve_class(string name, bool isGeneric, bool matchName) {
    GUARD(globalLock)

    for(Int i = 0; i < modules.size(); i++) {
        resolve_class(modules.get(i), name, isGeneric, matchName);
    }
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

