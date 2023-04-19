//
// Created by bknun on 9/18/2022.
//

#ifndef SHARP_SHARP_TYPE_H
#define SHARP_SHARP_TYPE_H


#include "../../core/data_type.h"
#include "sharp_class.h"
#include "sharp_function.h"

struct sharp_type {
    sharp_function *fun;
    sharp_class *sc;
    data_type type;
};

void init_struct(sharp_type *st);


#endif //SHARP_SHARP_TYPE_H
