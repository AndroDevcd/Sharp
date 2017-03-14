//
// Created by BraxtonN on 2/17/2017.
//

#ifndef SHARP_OBJECT_H
#define SHARP_OBJECT_H

#include <cstdint>
#include "../alloc/mark.h"
#include "../../../stdimports.h"
#include "string.h"
#include "../internal/Monitor.h"

#define ref_cap 0x16e

class ArrayObject;
class ClassObject;
class Reference;

enum Type {
    nativeint,
    nativeshort,
    nativelong,
    nativechar,
    nativebool,
    nativefloat,
    nativedouble,
    nativestring,
    classobject,
    arrayobject,
    refrenceobject,
    nilobject
};

/* native object */
struct Object {
    double prim;
    nString str;
};

/* Objects stored in memory */
class gc_object {
public:
    gc_object();

    gc_object(Type type);

    Reference* refs[ref_cap];
    int refCount;

    gc_mark mark;
    Type type;
    Object* obj;
    ArrayObject* arry;
    ClassObject* klass;
    Reference* ref;
    Monitor* monitor;

    void free();

    void invalidate();

    void inv_reference(Reference *pReference);

    void copy_object(gc_object *pObject);
};

#endif //SHARP_OBJECT_H
