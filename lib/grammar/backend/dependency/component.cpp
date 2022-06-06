//
// Created by BNunnally on 10/3/2021.
//

#include "component.h"
#include "../types/types.h"
#include "../operation/operation.h"
#include "../../compiler_info.h"

uInt uniqueComponentId = 0;

void type_definition::copy(const type_definition &c) {
    id = c.id;
    name = c.name;
    representation = c.representation;
    location = c.location;

    if(c.accessor)
        accessor = new sharp_field(*c.accessor);

    if(c.scheme)
        scheme = new operation_schema(*c.scheme);

    if(c.type)
        type = new sharp_type(*c.type);
}

void type_definition::free() {
    delete accessor; accessor = NULL;
    delete scheme;scheme = NULL;
    delete type; type = NULL;
}

void type_definition::copy_type(sharp_type *type) {
    this->type = new sharp_type(*type);
}

void component::copy(const component &c) {
    name = c.name;
    location = c.location;

    for(Int i = 0; i < c.named.size(); i++) {
        named.add(new type_definition(*c.named.get(i)));
    }

    for(Int i = 0; i < c.unnamed.size(); i++) {
        unnamed.add(new type_definition(*c.unnamed.get(i)));
    }
}

void component::free() {
    deleteList(named);
    deleteList(unnamed);
}
