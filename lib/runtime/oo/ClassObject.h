//
// Created by BraxtonN on 2/12/2018.
//

#ifndef SHARP_CLASSOBJECT_H
#define SHARP_CLASSOBJECT_H

#include "../../../stdimports.h"
#include "string.h"
#include "Field.h"

struct Method;

class ClassObject {
public:
    ClassObject()
            :
            name(),
            fields(NULL),
            methods(NULL),
            owner(NULL),
            super(NULL),
            interfaces(NULL),
            serial(-1),
            totalFieldCount(0),
            instanceFields(0),
            staticFields(0),
            methodCount(0),
            interfaceCount(0)
    {
        name.init();
    }

    void init() { ClassObject(); }

    ClassObject& operator=(const ClassObject& klass) {
        name = klass.name;
        fullName = klass.fullName;
        fields = klass.fields;
        methods = klass.methods;
        owner = klass.owner;
        serial = klass.serial;
        super = klass.super;
        totalFieldCount = klass.totalFieldCount;
        instanceFields = klass.instanceFields;
        staticFields = klass.staticFields;
        methodCount = klass.methodCount;

        return *this;
    }

    String name;
    String fullName;
    Field *fields; // TODO: have compiler format the fields as follows {[instance Fields] [static Fields]}
    Method **methods;
    ClassObject *owner, *super;
    ClassObject **interfaces;
    uInt
        serial,
        staticFields,
        instanceFields,
        totalFieldCount,
        methodCount,
        interfaceCount;

    void free();

    Field* getfield(string name);
    uInt fieldindex(string name);

    bool isClassRelated(ClassObject *klass);
};

#endif //SHARP_CLASSOBJECT_H
