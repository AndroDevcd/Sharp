//
// Created by BraxtonN on 2/17/2017.
//

#ifndef SHARP_CLASSOBJECT_H
#define SHARP_CLASSOBJECT_H

#include "../../../stdimports.h"
#include "string.h"

class Field;
class gc_object;
class Method;

class ClassObject {
public:
    ClassObject()
    :
            name(),
            flds(NULL),
            methods(NULL),
            super(NULL),
            id(-1),
            fieldCount(0),
            methodCount(0),
            fields(NULL)
    {
    }

    ClassObject(string name, Field* fields, int64_t fc,
                Method* methods, int64_t mc, ClassObject* super,
                int64_t id)
    :
            name(name),
            flds(fields),
            methods(methods),
            super(super),
            id(id),
            fieldCount(fc),
            methodCount(mc),
            fields(NULL)
    {
    }

    nString name;
    Field* flds;
    gc_object* fields;
    Method* methods;
    ClassObject* super;
    int64_t id, fieldCount, methodCount;

    void free();

    ClassObject *newdup();
    gc_object* get_field(int64_t);
};


#endif //SHARP_CLASSOBJECT_H
