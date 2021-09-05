//
// Created by BNunnally on 8/31/2021.
//
#include "access_flag.h"

void set_flag(uInt &flags, access_flag flag, bool enable) {
    flags ^= (-(unsigned long)enable ^ flags) & (1UL << flag);
}

bool check_flag(uInt flags, access_flag flag) {
    return ((flags >> flag) & 1U);
}
