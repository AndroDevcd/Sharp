//
// Created by BraxtonN on 2/12/2018.
//

#ifndef SHARP_FIELD_H
#define SHARP_FIELD_H

#include "../../../stdimports.h"
#include "string.h"
#include "../../grammar/DataType.h"

class ClassObject;

class Field {
public:
    Field()
            :
            name(),
            fullName()
    {
    }

    void init(string name, string fullName, int64_t addr, DataType type, uInt accessFlags, bool arrayType,
              ClassObject* owner, ClassObject *klass, bool tls = false)
    {
        this->name.init();
        this->fullName.init();
        this->name = name;
        this->fullName = fullName;
        this->address = addr;
        this->type=type;
        this->accessFlags=accessFlags;
        this->isArray=arrayType;
        this->threadLocal = tls;
        this->owner = owner;
        this->klass = klass;
    }

    void operator=(Field& field) {

        this->address = field.address;
        this->name = field.name;
        this->fullName = field.fullName;
        this->type = field.type;
        this->owner = field.owner;
        this->accessFlags = field.accessFlags;
        this->isArray=field.isArray;
        this->threadLocal = field.threadLocal;
        this->klass = field.klass;
    }

    String name, fullName;
    int64_t address;
    DataType type;
    uInt accessFlags;
    bool isArray;
    bool threadLocal;
    ClassObject* owner, *klass;

    void free() {
        owner = NULL;
        name.free();
        fullName.free();
    }
};

#define IS_STATIC(accessFlags) ((accessFlags >> 6) & 1UL)


#endif //SHARP_FIELD_H
