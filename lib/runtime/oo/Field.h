//
// Created by BraxtonN on 2/24/2017.
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

    void operator=(Field& field) {

        this->id = field.id;
        this->name = field.name;
        this->type = field.type;
        this->owner = field.owner;
        this->isstatic = field.isstatic;
    }

    nString name;
    int64_t id;
    int type;
    bool isstatic;
    ClassObject* owner;

    void free() {
        owner = NULL;
        name.free();
    }
};


#endif //SHARP_FIELD_H
