//
// Created by BNunnally on 9/1/2021.
//

#include "sharp_function.h"
#include "types.h"
#include "../dependency/dependancy.h"
#include "../../taskdelegator/task_delegator.h"
#include "../../compiler_info.h"
#include "../operation/operation.h"
#include "../finalizer/generation/code/code_info.h"

bool is_fully_qualified_function(sharp_function* function) {
    if(!function->parameters.empty()) {
        for(Int i = 0; i < function->parameters.size(); i++) {
            sharp_field *parameter = function->parameters.get(i);
            if(parameter->type.type >= type_any)
                return false;
        }
    }

    return function->returnType.type < type_any;
}

void fully_qualify_function(sharp_function* function, sharp_function* qualifiedFunction) {
    if(!function->parameters.empty() && function->parameters.size() == qualifiedFunction->parameters.size()) {
        for(Int i = 0; i < function->parameters.size(); i++) {
            sharp_field *parameter = function->parameters.get(i);
            if(parameter->type.type >= type_any)
                parameter->type.copy(qualifiedFunction->parameters.get(i)->type);
        }

        if(function->returnType.type >= type_any) {
            function->returnType.copy(qualifiedFunction->returnType);
        }
    }
}

bool create_function(
        sharp_class *sc,
        uInt flags,
        function_type type,
        string &name,
        bool checkBaseClass,
        List<sharp_field*> &params,
        sharp_type &returnType,
        Ast *createLocation,
        sharp_function *&createdFun) {

    GUARD(globalLock)
    sharp_function *sf;
    if((sf = resolve_function(name, sc, params, type,
                              0, createLocation, checkBaseClass, false)) == NULL) {
        sf = new sharp_function(name, sc,
                                impl_location(currThread->currTask->file, createLocation),
                                flags, createLocation, params, returnType, type);

        if(check_flag(sc->flags, flag_global)) {
            currThread->currTask->file->functions.add(sf);
        }

        sc->functions.add(sf);
        createdFun = sf;
        return true;
    } else {
        if(currThread->currTask->file->errors->createNewError(
                PREVIOUSLY_DEFINED, createLocation, "function `" + name +
                                                    "` is already defined"))
            print_impl_location(sf->name, "function", sf->implLocation);
        return false;
    }
}

bool create_function(
        sharp_class *sc,
        uInt flags,
        function_type type,
        string &name,
        bool checkBaseClass,
        List<sharp_field*> &params,
        sharp_type &returnType,
        Ast *createLocation) {

    sharp_function *ignoreFun = NULL;
    return create_function(sc, flags, type, name, checkBaseClass,
                    params, returnType, createLocation, ignoreFun);
}

void create_default_constructor(sharp_class *sc, uInt flags, Ast *createLocation) {

    sharp_type type(type_nil);
    List<sharp_field*> params;
    if(resolve_function(sc->name, sc, params, constructor_function,
                        0, createLocation, false, false) == NULL) {
        auto *sf = new sharp_function(sc->name, sc,
                                      impl_location(currThread->currTask->file, createLocation),
                                      flags, createLocation, params,
                                      type, constructor_function);

        GUARD(globalLock)
        currThread->currTask->file->functions.add(sf);
        sc->functions.add(sf);
    }
}

string parameters_to_str(List<sharp_field*> &parameters) {
    stringstream ss;
    ss << "(";

    for(Int i = 0; i < parameters.size(); i++) {
        ss << type_to_str(parameters.get(i)->type);

        if((i + 1) < parameters.size()) {
            ss << ", ";
        }
    }

    ss << ")";
    return ss.str();
}

string function_to_str(sharp_function *fun) {
    stringstream ss;
    ss << fun->name << parameters_to_str(fun->parameters);
    ss << ": " << type_to_str(fun->returnType) << "";

    return ss.str();
}

void set_full_name(sharp_function *sf) {
    sf->fullName = sf->owner->fullName + "." + sf->name;
}

bool function_parameters_match(
        List<sharp_field*> &comparer,
        List<sharp_field*> &comparee,
        bool explicitMatch,
        uInt excludedMatches) {
    if(comparer.size() != comparee.size()) return false;

    for(Int i = 0; i < comparer.size(); i++) {
        if(explicitMatch && !is_match_normal(is_explicit_type_match(comparer.get(i)->type, comparee.get(i)->type))) {
            return false;
        } else if(!explicitMatch){
            if(!is_match_normal(is_explicit_type_match(comparer.get(i)->type, comparee.get(i)->type))
                 && !is_match(is_implicit_type_match(comparer.get(i)->type, comparee.get(i)->type, excludedMatches))) {
                return false;
            }
        }
    }

    return true;
}

void sharp_function::free() {
    dependencies.free();
    if(!directlyCopyParams) deleteList(parameters);
    deleteList(locals);
    deleteList(aliases);
    deleteList(labels);
    delete scheme; scheme = NULL;
    delete ci; ci = NULL;
}

void sharp_function::copy_parameters(const List<sharp_field *> &params) {
    for(Int i = 0; i < params.size(); i++)
        parameters.add(new sharp_field(*params.get(i)));
}

void sharp_function::copy_locals(const List<sharp_field *> &localFields) {
    for(Int i = 0; i < locals.size(); i++)
        locals.add(new sharp_field(*localFields.get(i)));
}

void sharp_function::copy_scheme(operation_schema *operations) {
    if(operations != NULL) {
        delete scheme;
        scheme = new operation_schema(*operations);
    }
}
