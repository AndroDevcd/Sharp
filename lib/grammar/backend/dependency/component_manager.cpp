//
// Created by BNunnally on 10/3/2021.
//

#include "component_manager.h"
#include "../types/types.h"
#include "../../taskdelegator/task_delegator.h"
#include "../../settings/settings.h"

component_type* get_component(
        component_manager &manager,
        sharp_type &comparer,
        Ast *resolveLocation) {

    bool foundComponent = false;
    component_type *ct = NULL;
    for(Int i = 0; i < manager.components.size(); i++) {
        component *c = manager.components.get(i);

        if((ct = get_component(comparer, c)) != NULL) {
            if(foundComponent && resolveLocation != NULL) {
                create_new_warning(GENERIC, __w_dep, resolveLocation->line, resolveLocation->col,
                        "multiple components found with type `" + type_to_str(comparer) + "`, please explicitly specify component to avoid incorrect type injection.");
            }

            foundComponent = true;
        }
    }

    return ct;
}

component_type* get_component(
        component_manager &manager,
        sharp_type &comparer,
        string &componentName) {
    for(Int i = 0; i < manager.components.size(); i++) {
        component *c = manager.components.get(i);
        component_type *ct;

        if(c->name == componentName) {
            if ((ct = get_component(comparer, c)) != NULL) {
                return ct;
            }
        }
    }

    return NULL;
}

component_type* get_component(
        component_manager &manager,
        string &subComponentName,
        string &componentName) {
    for(Int i = 0; i < manager.components.size(); i++) {
        component *c = manager.components.get(i);

        if(c->name == componentName) {
            return get_component(subComponentName, c);
        }
    }

    return NULL;
}

component_type* get_component(
        string &subComponentName,
        component *c) {
    for(Int i = 0; i < c->named.size(); i++) {
        component_type *ct = c->named.get(i);

        if(ct->name == subComponentName) {
            return ct;
        }
    }

    return NULL;
}

component_type* get_component(
        sharp_type &comparer,
        component *c) {
    for(Int i = 0; i < c->unnamed.size(); i++) {
        component_type *ct = c->unnamed.get(i);

        if(ct->type != NULL
            && is_explicit_type_match(comparer, *ct->type)) {
            return ct;
        }
    }

    return NULL;
}

void component_manager::free() {
    deleteList(components);
}
