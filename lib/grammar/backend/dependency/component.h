//
// Created by BNunnally on 10/3/2021.
//

#ifndef SHARP_COMPONENT_H
#define SHARP_COMPONENT_H

#include "../../../../stdimports.h"
#include "../../List.h"
#include "../meta_data.h"

extern uInt uniqueComponentId;

struct sharp_field;
struct operation_scheme;
struct sharp_type;

enum component_representation {
    single_component,
    factory_component
};

struct component_type {
    component_type()
    :
        id(-1),
        name(""),
        accessor(NULL),
        scheme(NULL),
        type(NULL),
        location(),
        representation(single_component)
    {}

    component_type(string &name, impl_location &location)
    :
            id(uniqueComponentId++),
            name(""),
            accessor(NULL),
            scheme(NULL),
            type(NULL),
            location(location),
            representation(single_component)
    {
        this->name = name;
    }

    component_type(
            string &name,
            sharp_type *type,
            component_representation representation,
            impl_location &location)
    :
            id(uniqueComponentId++),
            name(""),
            accessor(NULL),
            scheme(NULL),
            type(NULL),
            location(location),
            representation(representation)
    {
        this->name = name;
        copy_type(type);
    }

    component_type(const component_type &c)
    :
      id(-1),
      name(""),
      accessor(NULL),
      scheme(NULL),
      type(NULL),
      location(),
      representation(single_component)
    {
        copy(c);
    }

    void copy(const component_type &c);
    void copy_type(sharp_type *);

    ~component_type() {
        free();
    }

    void free();

    uInt id;
    string name;
    sharp_field *accessor;
    operation_scheme *scheme;
    sharp_type *type;
    component_representation representation;
    impl_location location;
};

/**
 *
 * @field name
 * Name of the component
 *
 * @field named
 * Named types can only be used when they are called upon
 *
 * @field unnamed
 * Unnamed types are the only types that are considered when searching for types
 * in the dependency injection framework
 */
struct component {
    component()
    :
        name(""),
        location(),
        named(),
        unnamed()
    {}

    component(const component &c)
    :
            name(""),
            location(),
            named(),
            unnamed()
    {
        copy(c);
    }

    component(string &name, impl_location location)
            :
            name(""),
            location(location),
            named(),
            unnamed()
    {
        this->name = name;
    }

    void copy(const component &c);

    ~component() {
        free();
    }

    void free();

    string name;
    List<component_type*> named;
    List<component_type*> unnamed;
    impl_location location;
};


#endif //SHARP_COMPONENT_H
