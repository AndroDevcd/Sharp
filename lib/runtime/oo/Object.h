//
// Created by BraxtonN on 2/17/2017.
//

#ifndef SHARP_OBJECT_H
#define SHARP_OBJECT_H

#include <cstdint>
#include "../alloc/mark.h"
#include "../../../stdimports.h"
#include "string.h"

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

    union {
        bool nBool;
        int8_t nChar;
        int16_t nShort;
        int32_t nInt;
        int64_t nLong;
        float nFloat;
        double nDouble;
    };
    nString str;
};

/* Objects stored in memory */
class gc_object {
public:
    gc_object();

    gc_object(Type type);

    list<Reference*>* refs;

    gc_mark mark;
    Type type;
    Object* obj;
    ArrayObject* arry;
    ClassObject* klass;
    Reference* ref;

    void free();

    void invalidate();

    void inv_reference(Reference *pReference);
};

#endif //SHARP_OBJECT_H
