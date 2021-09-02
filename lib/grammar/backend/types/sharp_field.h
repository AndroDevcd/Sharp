//
// Created by BNunnally on 9/1/2021.
//

#ifndef SHARP_SHARP_FIELD_H
#define SHARP_SHARP_FIELD_H

#include "../../../../stdimports.h"
#include "../meta_data.h"
#include "../access_flag.h"
#include "sharp_type.h"

struct sharp_class;

struct sharp_field {
    sharp_field()
    :
        name(""),
        fullName(""),
        owner(NULL),
        implLocation(),
        dependencies(),
        flags(flag_none),
        ast(NULL)
    {}

    sharp_field(const sharp_field &sf)
    :
        name(sf.name),
        fullName(sf.fullName),
        owner(sf.owner),
        implLocation(sf.implLocation),
        dependencies(sf.dependencies),
        type(sf.type),
        flags(sf.flags),
        ast(sf.ast)
    {}

    ~sharp_field() {
        free();
    }

    void free() {
        dependencies.free();
    }

    string name;
    string fullName;
    sharp_class *owner;
    impl_location implLocation;
    List<dependency> dependencies;
    sharp_type type;
    uInt flags;
    Ast* ast;
};


#endif //SHARP_SHARP_FIELD_H
