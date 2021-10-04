//
// Created by BNunnally on 10/3/2021.
//

#ifndef SHARP_COMPONENT_MANAGER_H
#define SHARP_COMPONENT_MANAGER_H

#include "component.h"

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

component_type* get_component(component_manager &, sharp_type &, Ast *resolveLocation = NULL);
component_type* get_component(component_manager &, sharp_type &, string &);
component_type* get_component(component_manager &, string &, string &);
component_type* get_component(sharp_type &, component*);
component_type* get_component(string &, component*);

#endif //SHARP_COMPONENT_MANAGER_H