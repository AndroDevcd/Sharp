//
// Created by BNunnally on 9/8/2021.
//

#ifndef SHARP_GENERIC_TYPE_IDENTIFIER_H
#define SHARP_GENERIC_TYPE_IDENTIFIER_H

#include "../../../../stdimports.h"
#include "sharp_type.h"

struct generic_type_identifier {
    generic_type_identifier()
    :
        type(),
        name(""),
        baseClass(NULL)
    {}

    generic_type_identifier(string name)
    :
            type(),
            name(name),
            baseClass(NULL)
    {}

    generic_type_identifier(
            string name,
            sharp_type type,
            sharp_class *base)
    :
            type(type),
            name(name),
            baseClass(base)
    {}

    generic_type_identifier(
            const generic_type_identifier &gt)
    :
            type(gt.type),
            name(gt.name),
            baseClass(gt.baseClass)
    {}

    string name;
    sharp_type type;
    sharp_class *baseClass; // required base class if applicable
};

#endif //SHARP_GENERIC_TYPE_IDENTIFIER_H
