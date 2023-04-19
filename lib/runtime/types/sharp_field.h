//
// Created by bknun on 9/18/2022.
//

#ifndef SHARP_SHARP_FIELD_H
#define SHARP_SHARP_FIELD_H

#include "../../../stdimports.h"
#include "../../core/data_type.h"

struct sharp_class;
struct sharp_function;
struct sharp_type;

struct sharp_field {
    int32_t address;
    int32_t guid;
    string name;
    string fullName;
    sharp_class *owner;
    uint32_t flags;
    sharp_type *type;
    bool isArray;
    bool threadLocal;
};

void init_struct(sharp_field *sf);

#endif //SHARP_SHARP_FIELD_H
