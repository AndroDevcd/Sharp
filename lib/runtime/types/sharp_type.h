//
// Created by bknun on 9/18/2022.
//

#ifndef SHARP_SHARP_TYPE_H
#define SHARP_SHARP_TYPE_H


#include "../../core/data_type.h"
#include "sharp_class.h"
#include "sharp_function.h"

struct object;

struct sharp_type {
    sharp_function *fun;
    sharp_class *sc;
    data_type type;
};

CXX11_INLINE void init_struct(sharp_type *st);
bool is_type(object*, Int type);

#endif //SHARP_SHARP_TYPE_H
