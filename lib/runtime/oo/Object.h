//
// Created by BraxtonN on 2/17/2017.
//

#ifndef SHARP_OBJECT_H
#define SHARP_OBJECT_H

#include <cstdint>
#include "../alloc/mark.h"

class ArrayObject;
class ClassObject;

enum Type {
    nativeint,
    nativeshort,
    nativelong,
    nativechar,
    nativebool,
    nativefloat,
    nativedouble,
    classobject,
    arrayobject,
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
};

/* Objects stored in memory */
class gc_object {
public:
    gc_object();

    gc_object(Type type);

    int32_t refCounter;
    gc_mark mark;
    Type type;
    Object* obj;
    ArrayObject* arry;
    ClassObject* klass;

    void free();
};

#endif //SHARP_OBJECT_H
