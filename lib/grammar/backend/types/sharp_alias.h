//
// Created by BNunnally on 9/6/2021.
//

#ifndef SHARP_SHARP_ALIAS_H
#define SHARP_SHARP_ALIAS_H

#include "../../../../stdimports.h"
#include "sharp_type.h"
#include "../dependency/dependancy.h"
#include "../access_flag.h"
#include "../meta_data.h"

void set_full_name(sharp_alias*);

struct sharp_alias {
    sharp_alias()
    :
        name(""),
        fullName(""),
        owner(NULL),
        type(),
        flags(flag_none),
        dependencies(),
        location()
    {}

    sharp_alias(string name, sharp_class *owner, uInt flags, impl_location location)
    :
        name(name),
        owner(owner),
        type(),
        flags(flags),
        location(location)
    {
        set_full_name(this);
    }

    sharp_alias(const sharp_alias &alias)
    :
            name(alias.name),
            type(alias.type),
            owner(alias.owner),
            fullName(alias.fullName),
            dependencies(alias.dependencies),
            flags(alias.flags),
            location(alias.location)
    {}

    sharp_alias(string name, sharp_class *owner, uInt flags, sharp_type type, impl_location location)
    :
            name(name),
            owner(owner),
            dependencies(),
            type(type),
            flags(flags),
            location(location)
    {
        set_full_name(this);
    }

    ~sharp_alias()
    {
        free();
    }

    void free();

    string name;
    string fullName;
    sharp_class *owner;
    sharp_type type;
    uInt flags;
    impl_location location;
    List<dependency> dependencies;
};

sharp_alias* create_alias(sharp_file*, sharp_class*, string, uInt, Ast*);
sharp_alias* create_alias(sharp_file*, sharp_module*, string, uInt, Ast*);


#endif //SHARP_SHARP_ALIAS_H
