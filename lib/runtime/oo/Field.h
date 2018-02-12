//
// Created by BraxtonN on 2/12/2018.
//

#ifndef SHARP_FIELD_H
#define SHARP_FIELD_H

#include "../../../stdimports.h"
#include "string.h"

class ClassObject;

class Field {
public:
    Field()
            :
            name()
    {
    }

    void init(string name, int64_t id, int type, bool _static, bool arry,
              ClassObject* owner)
    {
        this->name.init();
        this->name = name;
        this->serial = id;
        this->type=type;
        this->isStatic=_static;
        this->isArray=arry;
    }

    void operator=(Field& field) {

        this->serial = field.serial;
        this->name = field.name;
        this->type = field.type;
        this->owner = field.owner;
        this->isStatic = field.isStatic;
    }

    native_string name;
    int64_t serial;
    int type;
    bool isStatic;
    bool isArray;
    ClassObject* owner;

    void free() {
        owner = NULL;
        name.free();
    }
};

#endif //SHARP_FIELD_H
