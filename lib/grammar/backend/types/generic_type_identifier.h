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
        baseType()
    {}

    generic_type_identifier(string name)
    :
            type(),
            name(name),
            baseType()
    {}

    generic_type_identifier(
            string name,
            sharp_type type,
            sharp_type baseType)
    :
            type(type),
            name(name)
    {
        if(type.type == type_field) {
            int i = 0;
        }
        this->baseType.copy(baseType);
    }

    generic_type_identifier(
            const generic_type_identifier &gt)
    :
            type(gt.type),
            name(gt.name),
            baseType()
    {
        this->baseType.copy(gt.baseType);
    }

    string name;
    sharp_type type;
    sharp_type baseType; // required base class if applicable
};

#endif //SHARP_GENERIC_TYPE_IDENTIFIER_H
