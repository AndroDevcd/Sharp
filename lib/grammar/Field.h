//
// Created by BraxtonN on 1/31/2017.
//

#ifndef SHARP_FIELD_H
#define SHARP_FIELD_H

#include "../../stdimports.h"
#include "NativeField.h"
#include "AccessModifier.h"

class ClassObject;

class Field {
public:
    Field(NativeField nf, uint64_t uid, string name, ClassObject* parent, AccessModifier modifiers[3])
    :
            nf(nf),
            uid(uid),
            name(name),
            parent(parent)
    {
        this->modifiers[0] = modifiers[0];
        this->modifiers[1] = modifiers[1];
        this->modifiers[2] = modifiers[2];
    }

    Field()
            :
            nf(fnof),
            uid(0),
            name("")
    {
    }

    const NativeField nf;
    const uint64_t uid;
    const string name;
    ClassObject* parent;
    AccessModifier modifiers[3]; // 3 max modifiers
};


#endif //SHARP_FIELD_H
