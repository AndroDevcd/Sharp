//
// Created by bknun on 9/18/2022.
//

#ifndef SHARP_SHARP_CLASS_H
#define SHARP_SHARP_CLASS_H

#include "../../../stdimports.h"
#include "../../core/data_type.h"

struct sharp_field;
struct sharp_function;

struct sharp_class {
    int32_t address;
    int32_t guid;
    string name;
    string fullName;
    sharp_class *owner;
    uint32_t flags;
    sharp_field *fields;
    sharp_function **methods;
    sharp_class *base;
    sharp_class **interfaces;
    Int staticFields;
    Int instanceFields;
    Int totalFieldCount;
    Int methodCount;
    Int interfaceCount;
};

void init_struct(sharp_class *sc);
bool are_classes_related(sharp_class*, sharp_class*);

#endif //SHARP_SHARP_CLASS_H
