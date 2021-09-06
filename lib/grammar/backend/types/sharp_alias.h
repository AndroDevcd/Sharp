//
// Created by BNunnally on 9/6/2021.
//

#ifndef SHARP_SHARP_ALIAS_H
#define SHARP_SHARP_ALIAS_H

#include "../../../../stdimports.h"
#include "sharp_type.h"
#include "../dependency/dependancy.h"

void set_full_name(sharp_alias*);

struct sharp_alias {
    sharp_alias()
    :
        name(""),
        fullName(""),
        owner(NULL),
        type(),
        dependencies()
    {}

    sharp_alias(string name, sharp_class *owner)
    :
        name(name),
        owner(owner),
        type()
    {
        set_full_name(this);
    }

    sharp_alias(const sharp_alias &alias)
    :
            name(alias.name),
            type(alias.type),
            owner(alias.owner),
            fullName(alias.fullName),
            dependencies(alias.dependencies)
    {}

    sharp_alias(string name, sharp_class *owner, sharp_type type)
    :
            name(name),
            owner(owner),
            dependencies(),
            type(type)
    {
        set_full_name(this);
    }

    string name;
    string fullName;
    sharp_class *owner;
    sharp_type type;
    List<dependency> dependencies;
};
// todo: create alias


#endif //SHARP_SHARP_ALIAS_H
