//
// Created by BNunnally on 10/3/2021.
//

#ifndef SHARP_COMPONENT_MANAGER_H
#define SHARP_COMPONENT_MANAGER_H

#include "component.h"
#include "../types/get_component_request.h"

struct component_manager {
    component_manager()
    :
        components()
    {}

    component_manager(const component_manager &c)
            :
            components()
    {
        for(Int i = 0; i < c.components.size(); i++) {
            components.add(new component(*c.components.get(i)));
        }
    }

    ~component_manager() {
        free();
    }

    void free();

    List<component*> components;
};

type_definition* get_type_definition(component_manager &, sharp_type &, get_component_request&);
type_definition* get_type_definition(component_manager &, sharp_type &, Ast *resolveLocation = NULL);
type_definition* get_type_definition(component_manager &, string &, Ast *resolveLocation = NULL);
type_definition* get_type_definition(component_manager &, string &, sharp_type&, Ast *resolveLocation = NULL);
type_definition* get_type_definition(component_manager &, sharp_type &, string &);
type_definition* get_type_definition(component_manager &, string &, string &);
type_definition* get_type_definition(sharp_type &, component*);
type_definition* get_type_definition(string &, component*);

component* get_component(component_manager &, string &);
component *create_component(component_manager&, string&, Ast*);
type_definition *create_type_definition(
        component*,
        string&,
        type_definition_rule,
        sharp_type&,
        Ast*);

#endif //SHARP_COMPONENT_MANAGER_H
