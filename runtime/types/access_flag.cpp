//
// Created by BNunnally on 8/31/2021.
//
#include "access_flag.h"

void set_flag(uInt &flags, access_flag flag, bool enable) {
    if(enable) flags ^= (uInt)flag;
    else flags |= (uInt)flag;
}

bool check_flag(uInt flags, access_flag flag) {
    return ((flags & flag) == flag);
}
