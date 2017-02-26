//
// Created by BraxtonN on 2/24/2017.
//

#ifndef SHARP_FIELD_H
#define SHARP_FIELD_H

#include "../../../stdimports.h"

class ClassObject;

class Field {
public:
    void operator=(Field& field) {

        this->id = field.id;
        this->name = field.name;
        this->type = field.type;
        this->owner = field.owner;
    }

    string name;
    int64_t id;
    int type;
    ClassObject* owner;

    void free() {
        owner = NULL;
    }
};


#endif //SHARP_FIELD_H
