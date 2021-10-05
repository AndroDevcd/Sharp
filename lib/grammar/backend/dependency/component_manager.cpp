//
// Created by BNunnally on 10/3/2021.
//

#include "component_manager.h"
#include "../types/types.h"
#include "../../taskdelegator/task_delegator.h"
#include "../../settings/settings.h"


component_type* get_sub_component(
        component_manager &manager,
        sharp_type &comparer,
        get_component_request &request) {
    if(request.componentName.empty() && request.subComponentName.empty()) {
        return get_sub_component(manager, comparer, NULL);
    } else {
        if(!request.componentName.empty()) {
            if(!request.componentName.empty()) {
                return get_sub_component(manager, request.subComponentName, request.componentName);
            } else {
                return get_sub_component(manager, comparer, request.componentName);
            }
        }
    }

    return NULL;
}

component_type* get_sub_component(
        component_manager &manager,
        sharp_type &comparer,
        Ast *resolveLocation) {
    GUARD(globalLock)

    bool foundComponent = false;
    component_type *ct = NULL;
    for(Int i = 0; i < manager.components.size(); i++) {
        component *c = manager.components.get(i);

        if((ct = get_sub_component(comparer, c)) != NULL) {
            if(foundComponent && resolveLocation != NULL) {
                create_new_warning(GENERIC, __w_dep, resolveLocation->line, resolveLocation->col,
                        "multiple components found with type `" + type_to_str(comparer) + "`, please explicitly specify component to avoid incorrect type injection.");
            }

            foundComponent = true;
        }
    }

    return ct;
}
component_type* get_sub_component(
        component_manager &manager,
        string &subComponentName,
        Ast *resolveLocation) {
    GUARD(globalLock)

    bool foundComponent = false;
    component_type *ct = NULL;
    for(Int i = 0; i < manager.components.size(); i++) {
        component *c = manager.components.get(i);

        if((ct = get_sub_component(subComponentName, c)) != NULL) {
            if(foundComponent && resolveLocation != NULL) {
                create_new_warning(GENERIC, __w_dep, resolveLocation->line, resolveLocation->col,
                                   "multiple type definitions found with name `" + subComponentName + "`, please explicitly specify component to avoid incorrect type injection.");
            }

            foundComponent = true;
        }
    }

    return ct;
}

component_type* get_sub_component(
        component_manager &manager,
        sharp_type &comparer,
        string &componentName) {
    GUARD(globalLock)
    for(Int i = 0; i < manager.components.size(); i++) {
        component *c = manager.components.get(i);
        component_type *ct;

        if(c->name == componentName) {
            if ((ct = get_sub_component(comparer, c)) != NULL) {
                return ct;
            }
        }
    }

    return NULL;
}

component_type* get_sub_component(
        component_manager &manager,
        string &subComponentName,
        string &componentName) {
    GUARD(globalLock)
    for(Int i = 0; i < manager.components.size(); i++) {
        component *c = manager.components.get(i);

        if(c->name == componentName) {
            return get_sub_component(subComponentName, c);
        }
    }

    return NULL;
}

component* get_component(
        component_manager &manager,
        string &componentName) {
    GUARD(globalLock)
    for(Int i = 0; i < manager.components.size(); i++) {
        component *c = manager.components.get(i);

        if(c->name == componentName) {
            return c;
        }
    }

    return NULL;
}

component_type* get_sub_component(
        string &subComponentName,
        component *c) {
    GUARD(globalLock)
    for(Int i = 0; i < c->named.size(); i++) {
        component_type *ct = c->named.get(i);

        if(ct->name == subComponentName) {
            return ct;
        }
    }

    return NULL;
}

component_type* get_sub_component(
        sharp_type &comparer,
        component *c) {
    GUARD(globalLock)
    for(Int i = 0; i < c->unnamed.size(); i++) {
        component_type *ct = c->unnamed.get(i);

        if(ct->type != NULL
            && is_explicit_type_match(comparer, *ct->type)) {
            return ct;
        }
    }

    return NULL;
}


component *create_component(
        component_manager &manager,
        string &componentName,
        Ast *ast) {
    GUARD(globalLock)
    component *c = get_component(manager, componentName);
    impl_location location(currThread->currTask->file, ast->line, ast->col);

    if(c == NULL) {
        manager.components.add(new component(componentName, location));
        return manager.components.last();
    } else {
        if(c->location != location) {
            currThread->currTask->file->errors->createNewError(PREVIOUSLY_DEFINED, ast->line, ast->col,
                    "component `" + componentName + "` has already been defined.");
            print_impl_location(componentName, "component", c->location);
        }

        return c;
    }
}


component_type* create_sub_component(
        component *comp,
        string &subCompName,
        component_representation representation,
        sharp_type &type,
        Ast *ast) {
    GUARD(globalLock)
    impl_location location(currThread->currTask->file, ast->line, ast->col);
    component_type *ct;

    if(subCompName.empty()) {
        if((ct = get_sub_component(type, comp)) != NULL) {
            currThread->currTask->file->errors->createNewError(PREVIOUSLY_DEFINED, ast->line, ast->col,
                                                               "component with type `" + type_to_str(type) + "` has already been defined.");
            print_impl_location(subCompName, "component", ct->location);
        } else {
            comp->unnamed.add(
                    new component_type(
                            subCompName,
                            &type,
                            representation,
                            location
                    )
            );

            return comp->unnamed.last();
        }
    } else {
        if((ct = get_sub_component(subCompName, comp)) != NULL) {
            currThread->currTask->file->errors->createNewError(PREVIOUSLY_DEFINED, ast->line, ast->col,
                "component with name `" + subCompName + "` has already been defined.");
            print_impl_location(subCompName, "component", ct->location);
        } else {
            comp->named.add(
                    new component_type(
                        subCompName,
                        &type,
                        representation,
                        location
                    )
            );

            return comp->named.last();
        }
    }

    return NULL;
}

void component_manager::free() {
    deleteList(components);
}
