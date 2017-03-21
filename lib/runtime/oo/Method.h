//
// Created by BraxtonN on 2/17/2017.
//

#ifndef SHARP_METHOD_H
#define SHARP_METHOD_H

#include "../../../stdimports.h"
#include "string.h"

class ClassObject;

class Method {

public:
    void operator=(Method& m) {
        name = m.name;
        owner = m.owner;
        id = m.id;
        entry = m.entry;
        retAdr = m.retAdr;
    }

    ClassObject* owner;
    nString name;
    int64_t id;
    int64_t entry, retAdr;
    int64_t locals;

    void free() {
        owner = NULL;
    }

};


#endif //SHARP_METHOD_H
