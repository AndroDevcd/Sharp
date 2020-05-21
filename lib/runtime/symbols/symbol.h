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
    :
        address(invalidAddr),
        guid(invalidAddr),
        name(),
        fullName(),
        owner(NULL),
        flags(0),
        type(UNDEFINED)
    {
    }

    void init() { Symbol(); }

    void free() {
        name.free();
        fullName.free();
    }

    Int address;
    Int guid;
    String name;
    String fullName;
    ClassObject *owner;
    Int flags;
    DataType type;
};

#endif //SHARP_SYMBOL_H
