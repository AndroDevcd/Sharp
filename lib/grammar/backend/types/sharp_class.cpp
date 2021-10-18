//
// Created by BNunnally on 8/31/2021.
//

#include "sharp_class.h"
#include "../dependency/dependancy.h"
#include "../../compiler_info.h"
#include "sharp_module.h"
#include "../../taskdelegator/task_delegator.h"
#include "sharp_function.h"
#include "sharp_alias.h"
#include "sharp_field.h"


void create_global_class() {
    GUARD(globalLock)

    uInt flags = 0;
    string name = global_class_name;
    set_flag(flags, flag_public, true);
    set_flag(flags, flag_global, true);
    if(resolve_class(currModule, name, false, false) == NULL) {
        auto gc = create_class(
                currThread->currTask->file,
                currModule, name, flags, class_normal, false,
                currThread->currTask->file->p->astAt(0)
        );

        create_default_constructor(gc, flag_public, gc->ast);
    }
}

string get_simple_name(sharp_class *sc) {
    string basicName = sc->name;
    if(basicName.find("<") != string::npos) {
        stringstream ss;
        for(int i = 0; i < basicName.size(); i++) {
            if(basicName[i] == '<')
                break;
            ss << basicName[i];
        }

        basicName = ss.str();
    }

    return basicName;
}

sharp_class* create_closure_class(
        sharp_file *file,
        sharp_module *module,
        sharp_function *function,
        Ast *ast) {
    string className = "closure_" + function->fullName;
    sharp_class *sc;

    if((sc = resolve_class(file, className, false, false)) != NULL) {
        return sc;
    } else return create_class(
            file, module, className, flag_public | flag_local,
            class_normal, false, ast);
}


sharp_class* create_generic_class(
        sharp_class *genericBlueprint,
        List<sharp_type> &genericTypes,
        bool &classCreated) {

    if(genericTypes.size() == genericBlueprint->genericTypes.size()) {
        sharp_class *sc;

        string typedName = genericBlueprint->name + "<";

        for(Int i = 0; i < genericTypes.size(); i++) {

            typedName += type_to_str(genericTypes.get(i));

            if((i + 1) < genericTypes.size()) {
                typedName += ", ";
            }
        }

        typedName += ">";

        if((sc = resolve_class(genericBlueprint->owner, typedName, false, false)) != NULL) {
            return sc;
        } else {
            List<generic_type_identifier> genericIdentifiers;
            for(Int i = 0; i < genericTypes.size(); i++) {
                generic_type_identifier *typeIdentifier = &genericBlueprint->genericTypes.get(i);
                sharp_type *type = &genericTypes.get(i);
                genericIdentifiers.add(generic_type_identifier(typeIdentifier->name, *type, NULL));

                if(typeIdentifier->baseClass) {
                    if(!(type->type == type_class
                        && is_implicit_type_match(typeIdentifier->baseClass, type->_class))) {
                        stringstream ss;
                        ss << "for generic class `" <<  genericBlueprint->fullName << "` type ("
                           << typeIdentifier->name << ") must contain base class `" << typeIdentifier->baseClass->fullName << "`";
                        currThread->currTask->file->errors->createNewError(GENERIC, genericBlueprint->ast->line, genericBlueprint->ast->col, ss.str());
                    }
                }
            }

            classCreated = true;
            sc = create_class(
                    currThread->currTask->file,
                    genericBlueprint->owner,
                    typedName,
                    genericBlueprint->flags,
                    genericBlueprint->type,
                    false,
                    genericBlueprint->ast);

            sc->genericTypes.addAll(genericIdentifiers);
            genericIdentifiers.free();
            return sc;
        }
    } else {
        stringstream ss;
        ss << "generic class `" <<  genericBlueprint->fullName << "` expected ("
            << genericBlueprint->genericTypes.size() << ") types but (" << genericTypes.size() << ") was provided";
        currThread->currTask->file->errors->createNewError(GENERIC, genericBlueprint->ast->line, genericBlueprint->ast->col, ss.str());
        return NULL;
    }
}

sharp_class* create_class(
        sharp_file *file,
        sharp_class* owner,
        string name,
        uInt flags,
        class_type type,
        bool isGeneric,
        Ast *ast) {
    impl_location location(file, 0, 0);
    sharp_class *sc = NULL;

    if(ast != NULL) {
        location.line = ast->line;
        location.col = ast->col;
    }

    if((sc = resolve_class(owner, name, false, false)) != NULL
       || (sc = resolve_class(owner, name, true, false)) != NULL) {
        GUARD(globalLock)
        if(file->errors->createNewError(PREVIOUSLY_DEFINED, ast, "child class `" + name +
                                    "` is already defined in class {" + sc->fullName + "}"))
            print_impl_location(sc->name, "class", sc->implLocation);
        return sc;
    } else {
        GUARD(globalLock)
        sc = new sharp_class(
                name, owner, owner->module,
                location, flags, ast, type
        );

        if(isGeneric) owner->generics.add(sc);
        else owner->children.add(sc);
        return sc;
    }
}

sharp_class* create_class(
        sharp_file *file,
        sharp_module* module,
        string name,
        uInt flags,
        class_type type,
        bool isGeneric,
        Ast *ast) {
    impl_location location(file, 0, 0);
    sharp_class *sc = NULL, *owner = NULL;

    if(ast != NULL) {
        location.line = ast->line;
        location.col = ast->col;
    }

    if((sc = resolve_class(module, name, false, false)) != NULL
        || (sc = resolve_class(module, name, true, false)) != NULL) {
        GUARD(globalLock)
        if(file->errors->createNewError(PREVIOUSLY_DEFINED, ast, "class `" + name +
                                  "` is already defined in module {" + module->name + "}"))
            print_impl_location(sc->name, "class", sc->implLocation);
        return sc;
    } else {
        owner = resolve_class(module, global_class_name, false, false);

        GUARD(globalLock)
        if(check_flag(flags, flag_global) && owner != NULL) {
            int i = 0;
        }
        sc = new sharp_class(
                name, owner, module,
                location, flags, ast, type
        );

        if(isGeneric) {
            if(owner != NULL) owner->generics.add(sc);
            genericClasses.add(sc);
            module->genericClasses.add(sc);
        } else {
            if(owner != NULL) owner->children.add(sc);
            classes.add(sc);
            module->classes.add(sc);
        }

        return sc;
    }

}

void sharp_class::free() {
    dependencies.free();
    genericTypes.free();
    interfaces.free();
    extensionFunctions.free();
    mutations.free();
    deleteList(children);
    deleteList(functions);
    deleteList(generics);
    deleteList(aliases);
    deleteList(fields);
}

bool is_explicit_type_match(sharp_class *comparer, sharp_class * comparee) {
    return comparer == comparee;
}

bool is_implicit_type_match(sharp_class *comparer, sharp_class *comparee) {
    return is_explicit_type_match(comparer, comparee) || is_class_related_to(comparer, comparee);
}

bool is_class_related_to(sharp_class *comparer, sharp_class *baseClass) {
    if(comparer->baseClass != NULL) {
        if(comparer->baseClass == baseClass) return true;
        else return is_class_related_to(comparer->baseClass, baseClass);
    }

    return false;
}

sharp_field* locate_field(
        string name,
        sharp_class *owner) {
    GUARD(globalLock)
    for(Int i = 0; i < owner->fields.size(); i++) {
        if(owner->fields.get(i)->name == name)
            return owner->fields.get(i);
    }

    return NULL;
}

generic_type_identifier* locate_generic_type(
        string name,
        sharp_class *owner) {
    GUARD(globalLock)
    for(Int i = 0; i < owner->genericTypes.size(); i++) {
        if(owner->genericTypes.get(i).name == name)
            return &owner->genericTypes.get(i);
    }

    return NULL;
}

bool locate_functions_with_name(
        string name,
        sharp_class *owner,
        Int functionType,
        bool checkBaseClass,
        List<sharp_function*> &results) {
    GUARD(globalLock)
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

bool locate_functions_with_type(
        sharp_class *owner,
        Int functionType,
        bool checkBaseClass,
        List<sharp_function*> &results) {
    GUARD(globalLock)
    for(Int i = 0; i < owner->functions.size(); i++) {
        sharp_function *fun = owner->functions.get(i);
        if(functionType == fun->type)
            results.add(fun);
    }

    if(checkBaseClass && owner->baseClass)
        return locate_functions_with_type(
                owner->baseClass, functionType,
                true, results);

    return !results.empty();
}