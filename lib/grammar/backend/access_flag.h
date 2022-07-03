//
// Created by BNunnally on 8/31/2021.
//

#ifndef SHARP_ACCESS_FLAG_H
#define SHARP_ACCESS_FLAG_H

#include "../../../stdimports.h"

enum access_flag {
    flag_none = 0x000,

    flag_public = 0x001,
    flag_private = 0x002,
    flag_protected = 0x004,

    // todo: add errors for non-nullable fields not being assigned
    flag_excuse = 0x008, // this flag excuses a non-nullable fields not being initialized

    flag_local = 0x0010,

    flag_const = 0x020,
    flag_static = 0x040,

    flag_stable = 0x080,
    flag_unstable = 0x100,

    flag_extension = 0x200,

    flag_native = 0x400,

    flag_global = 0x800
};

void set_flag(uInt &flags, access_flag flag, bool enable);
bool check_flag(uInt flags, access_flag flag);

#endif //SHARP_ACCESS_FLAG_H
