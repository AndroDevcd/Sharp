//
// Created by BNunnally on 9/1/2021.
//

#ifndef SHARP_SHARP_FIELD_H
#define SHARP_SHARP_FIELD_H

#include "../../../../stdimports.h"
#include "../meta_data.h"
#include "../access_flag.h"
#include "sharp_type.h"
#include "../dependency/dependancy.h"

struct sharp_class;

enum field_type {
    normal_field,
    tls_field
};

struct sharp_field {
    sharp_field()
    :
        name(""),
        fullName(""),
        owner(NULL),
        implLocation(),
        dependencies(),
        flags(flag_none),
        fieldType(normal_field),
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
        fieldType(sf.fieldType),
        ast(sf.ast)
    {}

    sharp_field(
            string name,
            sharp_class *owner,
            impl_location location,
            sharp_type type,
            uInt flags,
            field_type ft,
            Ast *ast)
    :
            name(name),
            fullName(""),
            owner(owner),
            implLocation(location),
            dependencies(),
            type(type),
            flags(flags),
            fieldType(ft),
            ast(ast)
    {
        set_full_name();
    }

    ~sharp_field() {
        free();
    }

    void free() {
        dependencies.free();
    }

    void set_full_name();

    string name;
    string fullName;
    sharp_class *owner;
    impl_location implLocation;
    List<dependency> dependencies;
    sharp_type type;
    field_type fieldType;
    uInt flags;
    Ast* ast;
};

sharp_field* create_field(
        sharp_file*,
        sharp_module*,
        string,
        uInt,
        sharp_type,
        field_type,
        Ast*);

sharp_field* create_field(
        sharp_file*,
        sharp_class*,
        string,
        uInt,
        sharp_type,
        field_type,
        Ast*);


#endif //SHARP_SHARP_FIELD_H
