//
// Created by BNunnally on 5/18/2020.
//

#ifndef SHARP_SYMBOL_H
#define SHARP_SYMBOL_H

#include "../../../stdimports.h"
#include "string.h"
#include "../../grammar/DataType.h"

extern int invalidAddr;
class ClassObject;
class Symbol {
public:
    Symbol()
    {
        init();
    }

    void init() {
        address = invalidAddr;
        guid = invalidAddr;
        owner = NULL;
        flags = 0;
        type = UNDEFINED;
        new (&name)string();
        new (&fullName)string();
    }

    void free() {
        name.clear();
        fullName.clear();
    }

    int32_t address;
    int32_t guid;
    string name;
    string fullName;
    ClassObject *owner;
    uint32_t flags;
    DataType type;
};

#endif //SHARP_SYMBOL_H
