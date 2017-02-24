//
// Created by BraxtonN on 2/17/2017.
//

#ifndef SHARP_META_H
#define SHARP_META_H

#include "../../stdimports.h"

class ClassObject;
class Field;
class Method;

/**
 * File and debugging info, line
 * by line debugging meta data
 */
class Meta {

};

class MetaClass {
public:
    MetaClass(ClassObject* c, int64_t sup)
    :
            super(sup),
            c(c)
    {
    }

    ClassObject* c;
    int64_t super;
};

class MetaField {
public:
    MetaField(Field* f, int64_t owner)
    :
            field(f),
            owner(owner)
    {
    }

    Field* field;
    int64_t owner;
};

#endif //SHARP_META_H
