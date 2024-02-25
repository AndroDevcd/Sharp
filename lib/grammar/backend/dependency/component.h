//
// Created by BNunnally on 10/3/2021.
//

#ifndef SHARP_COMPONENT_H
#define SHARP_COMPONENT_H

#include "../../../../stdimports.h"
#include "../../List.h"
#include "../meta_data.h"
#include "../dependency/dependancy.h"

extern uInt uniqueComponentId;

struct sharp_field;
struct operation_schema;
struct sharp_type;

enum type_definition_rule {
    single_type_definition,
    factory_type_definition
};

struct type_definition {
    type_definition()
    :
        id(-1),
        name(""),
        scheme(NULL),
        type(NULL),
        location(),
        dependencies(),
        representation(single_type_definition)
    {}

    type_definition(string &name, impl_location &location)
    :
            id(uniqueComponentId++),
            name(""),
            scheme(NULL),
            type(NULL),
            dependencies(),
            location(location),
            representation(single_type_definition)
    {
        this->name = name;
    }

    type_definition(
            string &name,
            sharp_type *type,
            type_definition_rule representation,
            impl_location &location)
    :
            id(uniqueComponentId++),
            name(""),
            scheme(NULL),
            type(NULL),
            dependencies(),
            location(location),
            representation(representation)
    {
        this->name = name;
        copy_type(type);
    }

    type_definition(const type_definition &c)
    :
      id(-1),
      name(""),
      scheme(NULL),
      type(NULL),
      location(),
      dependencies(),
      representation(single_type_definition)
    {
        copy(c);
    }

    void copy(const type_definition &c);
    void copy_type(sharp_type *);

    ~type_definition() {
        free();
    }

    void free();

    uInt id;
    string name;
    operation_schema *scheme;
    List<dependency> dependencies;
    sharp_type *type;
    type_definition_rule representation;
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
    List<type_definition*> named;
    List<type_definition*> unnamed;
    impl_location location;
};


#endif //SHARP_COMPONENT_H
