//
// Created by BNunnally on 9/1/2021.
//

#include "sharp_function.h"
#include "sharp_field.h"
#include "sharp_class.h"
#include "../dependency/dependancy.h"
#include "../../taskdelegator/task_delegator.h"
#include "../../compiler_info.h"

bool is_fully_qualified_function(sharp_function* function) {
    if(!function->parameters.empty()) {
        for(Int i = 0; i < function->parameters.size(); i++) {
            sharp_field *parameter = function->parameters.get(i);
            if(parameter->type.type >= type_any)
                return false;
        }
    }

    return true;
}

bool create_function(
        sharp_class *sc,
        uInt flags,
        function_type type,
        string &name,
        bool checkBaseClass,
        List<sharp_field*> params,
        sharp_type &returnType,
        Ast *createLocation) {

    GUARD(sc->mut)
    sharp_function *sf;
    if((sf = resolve_function(name, sc, params, type,
            0, createLocation, checkBaseClass, false)) == NULL) {
        sf = new sharp_function(name, sc,
                impl_location(currThread->currTask->file, createLocation),
                flags, createLocation, params, returnType, type);

        sc->functions.add(sf);
        return true;
    } else {
        currThread->currTask->file->errors->createNewError(
                PREVIOUSLY_DEFINED, createLocation, "function `" + name +
                            "` is already defined");
        print_impl_location(sf->name, "function", sf->implLocation);
        return false;
    }
}

void create_default_constructor(sharp_class *sc, uInt flags, Ast *createLocation) {

    List<sharp_field*> params;
    if(resolve_function(sc->name, sc, params, constructor_function,
                        0, createLocation, false, false) == NULL) {
        auto *sf = new sharp_function(sc->name, sc,
                                      impl_location(currThread->currTask->file, createLocation),
                                      flags, createLocation, params,
                                      sharp_type(type_nil), constructor_function);

        GUARD(sc->mut)
        sc->functions.add(sf);
    }
}

void set_full_name(sharp_function *sf) {
    sf->fullName = sf->owner->fullName + "." + sf->name;
}

bool function_parameters_match(List<sharp_field*> &comparer, List<sharp_field*> &comparee, bool explicitMatch) {
    if(comparer.size() != comparee.size()) return false;

    for(Int i = 0; i < comparer.size(); i++) {
        if(!(explicitMatch && is_explicit_type_match(comparer.get(i)->type, comparee.get(i)->type))
            && !(!explicitMatch && is_implicit_type_match(comparer.get(i)->type, comparee.get(i)->type, 0))) {
            return false;
        }
    }

    return false;
}

void sharp_function::free() {
    dependencies.free();
    deleteList(parameters);
}

void sharp_function::copy_parameters(List<sharp_field *> params) {
    for(Int i = 0; i < params.size(); i++)
        parameters.add(new sharp_field(*params.get(i)));
}