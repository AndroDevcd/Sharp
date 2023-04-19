//
// Created by bknun on 9/18/2022.
//

#include "sharp_type.h"

void init_struct(sharp_type *st) {
    st->type = type_undefined;
    st->fun = nullptr;
    st->sc = nullptr;
}
