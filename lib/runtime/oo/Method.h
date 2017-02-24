//
// Created by BraxtonN on 2/17/2017.
//

#ifndef SHARP_METHOD_H
#define SHARP_METHOD_H

#include "../../../stdimports.h"

class ClassObject;

class Method {

public:
    void operator=(Method& m) {
        name = m.name;
        owner = m.owner;
        id = m.id;
        entry = m.entry;
        ret = m.ret;
    }

    ClassObject* owner;
    string name;
    int64_t id;
    int64_t entry, ret;
};


#endif //SHARP_METHOD_H
