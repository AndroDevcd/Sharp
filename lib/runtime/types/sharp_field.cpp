//
// Created by bknun on 9/18/2022.
//

#include "sharp_field.h"

CXX11_INLINE void init_struct(sharp_field *sf) {
    sf->address = 0;
    sf->guid = 0;
    new (&sf->name) string();
    new (&sf->fullName) string();
    sf->owner = nullptr;
    sf->flags = 0;
    sf->reflect = nullptr;
    sf->type = nullptr;
    sf->isArray = false;
    sf->threadLocal = false;
}
