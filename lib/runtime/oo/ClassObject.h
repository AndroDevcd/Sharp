//
// Created by BraxtonN on 2/12/2018.
//

#ifndef SHARP_CLASSOBJECT_H
#define SHARP_CLASSOBJECT_H

#include "../../../stdimports.h"
#include "string.h"

class Field;
class Object;
class Method;

class ClassObject {
public:
    ClassObject()
            :
            name(),
            fields(NULL),
            methods(NULL),
            super(NULL),
            serial(-1),
            fieldCount(0),
            methodCount(0)
    {
    }

    ClassObject(string name, Field* fields, unsigned long fieldCount,
                ClassObject* super, unsigned long id)
            :
            name(name),
            fields(fields),
            methods(NULL),
            super(super),
            serial(id),
            fieldCount(fieldCount),
            methodCount(0)
    {
    }

    void operator=(const ClassObject& klass) {
        name = klass.name;
        fields = klass.fields;
        methods = klass.methods;
        super = klass.super;
        serial = klass.serial;
        fieldCount = klass.fieldCount;
        methodCount = klass.methodCount;
    }

    native_string name;
    Field* fields;
    unsigned long *methods;
    ClassObject* super;
    unsigned long serial, fieldCount, methodCount;

    void free();

    Field* getfield(string name);
    int64_t fieldindex(string name);

    void init();

    bool hasBaseClass(ClassObject *pObject);
};

#endif //SHARP_CLASSOBJECT_H
