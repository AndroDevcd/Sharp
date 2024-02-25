//
// Created by BNunnally on 10/3/2021.
//

#include "component_manager.h"
#include "../types/types.h"
#include "../../taskdelegator/task_delegator.h"
#include "../../settings/settings.h"


type_definition* get_type_definition(
        component_manager &manager,
        sharp_type &comparer,
        get_component_request &request) {
    component *primary = get_primary_component(&currThread->currTask->file->ctx);
    type_definition *td;

    if(request.componentName.empty() && request.typeDefinitionName.empty()) {

        if(primary != NULL
            && (td = get_type_definition(comparer, primary)) != NULL) {
            return td;
        } else return get_type_definition(manager, comparer, NULL);

    } else {
        if(!request.typeDefinitionName.empty()) {
            if(!request.componentName.empty()) {
                return get_type_definition(manager, request.typeDefinitionName, request.componentName);
            } else {
                if(primary != NULL
                   && (td = get_type_definition(request.typeDefinitionName, primary)) != NULL) {
                    return td;
                } else return  get_type_definition(manager, request.typeDefinitionName);
            }
        } else return get_type_definition(manager, comparer, request.componentName);
    }

    return NULL;
}

type_definition* get_type_definition(
        component_manager &manager,
        sharp_type &comparer,
        Ast *resolveLocation) {
    GUARD(globalLock)

    bool foundComponent = false;
    type_definition *ct = NULL;
    for(Int i = 0; i < manager.components.size(); i++) {
        component *c = manager.components.get(i);

        if((ct = get_type_definition(comparer, c)) != NULL) {
            if(foundComponent && resolveLocation != NULL) {
                create_new_warning(GENERIC, __w_dep, resolveLocation->line, resolveLocation->col,
                        "multiple components found with type `" + type_to_str(comparer) + "`, please explicitly specify component to avoid incorrect type injection.");
            }

            foundComponent = true;
        }
    }

    return ct;
}

type_definition* get_type_definition(
        component_manager &manager,
        string &typeDefinitionName,
        Ast *resolveLocation) {
    GUARD(globalLock)

    bool foundComponent = false;
    type_definition *td = NULL;
    for(Int i = 0; i < manager.components.size(); i++) {
        component *c = manager.components.get(i);

        if((td = get_type_definition(typeDefinitionName, c)) != NULL) {
            if(foundComponent && resolveLocation != NULL) {
                create_new_warning(GENERIC, __w_dep, resolveLocation->line, resolveLocation->col,
                                   "multiple type definitions found with name `" + typeDefinitionName + "`, please explicitly specify component to avoid incorrect type injection.");
            }

            foundComponent = true;
        }
    }

    return td;
}

type_definition* get_type_definition(
        component_manager &manager,
        string &typeDefinitionName,
        sharp_type &comparer,
        Ast *resolveLocation) {
    GUARD(globalLock)

    bool foundComponent = false;
    type_definition *td = NULL;
    for(Int i = 0; i < manager.components.size(); i++) {
        component *c = manager.components.get(i);

        if((td = get_type_definition(typeDefinitionName, c)) != NULL
           && is_explicit_type_match(comparer, *td->type)) {
            if(foundComponent && resolveLocation != NULL) {
                create_new_warning(GENERIC, __w_dep, resolveLocation->line, resolveLocation->col,
                                   "multiple type definitions found with name `" + typeDefinitionName + "`, please explicitly specify component to avoid incorrect type injection.");
            }

            foundComponent = true;
        }
    }

    return td;
}

type_definition* get_type_definition(
        component_manager &manager,
        sharp_type &comparer,
        string &componentName) {
    GUARD(globalLock)
    for(Int i = 0; i < manager.components.size(); i++) {
        component *c = manager.components.get(i);
        type_definition *ct;

        if(c->name == componentName || componentName == "?") {
            if ((ct = get_type_definition(comparer, c)) != NULL) {
                return ct;
            }
        }
    }

    return NULL;
}

type_definition* get_type_definition(
        component_manager &manager,
        string &typeName,
        string &componentName) {
    GUARD(globalLock)
    for(Int i = 0; i < manager.components.size(); i++) {
        component *c = manager.components.get(i);

        if(c->name == componentName) {
            return get_type_definition(typeName, c);
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

type_definition* get_type_definition(
        string &typeName,
        component *c) {
    GUARD(globalLock)
    for(Int i = 0; i < c->named.size(); i++) {
        type_definition *ct = c->named.get(i);

        if(ct->name == typeName) {
            return ct;
        }
    }

    return NULL;
}

type_definition* get_type_definition(
        sharp_type &comparer,
        component *c) {
    GUARD(globalLock)
    for(Int i = 0; i < c->unnamed.size(); i++) {
        type_definition *ct = c->unnamed.get(i);

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
            create_new_error(PREVIOUSLY_DEFINED, ast->line, ast->col,
                    "component `" + componentName + "` has already been defined.");
            print_impl_location(componentName, "component", c->location);
        }

        return c;
    }
}


type_definition* create_type_definition(
        component *comp,
        string &subCompName,
        type_definition_rule representation,
        sharp_type &type,
        Ast *ast) {
    GUARD(globalLock)
    impl_location location(currThread->currTask->file, ast->line, ast->col);
    type_definition *ct;

    if(subCompName.empty()) {
        if((ct = get_type_definition(type, comp)) != NULL) {
            create_new_error(PREVIOUSLY_DEFINED, ast->line, ast->col,
                     "type definition of type `" + type_to_str(type) + "` has already been defined.");
            print_impl_location(subCompName, "type definition", ct->location);
        } else {
            comp->unnamed.add(
                    new type_definition(
                            subCompName,
                            &type,
                            representation,
                            location
                    )
            );

            return comp->unnamed.last();
        }
    } else {
        if((ct = get_type_definition(subCompName, comp)) != NULL) {
            create_new_error(PREVIOUSLY_DEFINED, ast->line, ast->col,
                "type definition with name `" + subCompName + "` has already been defined.");
            print_impl_location(subCompName, "type definition", ct->location);
        } else {
            comp->named.add(
                    new type_definition(
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

void component_manager::copy(const component_manager &c) {
    free();

    for(Int i = 0; i < c.components.size(); i++) {
        components.add(new component(*c.components.get(i)));
    }
}
