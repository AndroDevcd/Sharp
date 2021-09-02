//
// Created by BNunnally on 8/31/2021.
//

#include "sharp_class.h"
#include "../dependency/dependancy.h"
#include "../../compiler_info.h"
#include "sharp_module.h"
#include "../../taskdelegator/task_delegator.h"
#include "sharp_function.h"


void create_global_class() {
    GUARD(globalLock)

    uInt flags;
    string name = global_class_name;
    set_flag(flags, flag_public, true);
    set_flag(flags, flag_global, true);
    sharp_class *gc;
    if((gc = resolve_class(currModule, name, false, false)) == NULL) {
        gc = create_class(
                currThread->currTask->file,
                currModule, name, flags,
                currThread->currTask->file->p->astAt(0)
        );
        // TODO: add default construxtor
    }
}


sharp_class* create_class(
        sharp_file *file,
        sharp_module* module,
        string name,
        uInt flags,
        Ast *ast) {
    impl_location location(file, 0, 0);
    sharp_class *sc = NULL, *owner = NULL;

    if(ast != NULL) {
        location.line = ast->line;
        location.col = ast->col;
    }

    if((sc = resolve_class(module, name, false, false)) != NULL
        || (sc = resolve_class(module, name, true, false)) != NULL) {
        file->errors->createNewError(PREVIOUSLY_DEFINED, ast->line, ast->col, "class `" + name +
                                  "` is already defined in module {" + module->name + "}");
        print_impl_location(sc, sc->implLocation);
        return sc;
    } else {
        owner = resolve_class(global_class_name, false, false);
        sc = new sharp_class(
                name, owner, module,
                location, flags, ast
        );

        GUARD(globalLock)
        if(owner != NULL) owner->children.add(sc);
        classes.add(sc);
        return sc;
    }

}

void sharp_class::free() {
    dependencies.free();
    for(Int i = 0; i < children.size(); i++) {
        delete children.at(i);
    }

    for(Int i = 0; i < functions.size(); i++) {
        delete functions.at(i);
    }

    children.free();
    functions.free();
}

bool is_explicit_type_match(sharp_class *comparer, sharp_class * comparee) {
    return comparer == comparee;
}

bool is_implicit_type_match(sharp_class *comparer, sharp_class *comparee) {
    return is_class_related_to(comparer, comparee);
}

bool is_class_related_to(sharp_class *comparer, sharp_class *baseClass) {
    if(comparer->baseClass != NULL) {
        if(comparer->baseClass == baseClass) return true;
        else return is_class_related_to(comparer->baseClass, baseClass);
    }

    return false;
}

bool locate_functions_with_name(
        string name,
        sharp_class *owner,
        Int functionType,
        bool checkBaseClass,
        List<sharp_function*> &results) {
    for(Int i = 0; i < owner->functions.size(); i++) {
        sharp_function *fun = owner->functions.get(i);
        if(name == fun->name &&
            (functionType == undefined_function || functionType == fun->type))
            results.add(fun);
    }

    if(checkBaseClass && owner->baseClass)
        return locate_functions_with_name(
                name, owner->baseClass, functionType,
                true, results);

    return !results.empty();
}