//
// Created by bknun on 7/15/2023.
//

#ifndef SHARP_LIBRARY_H
#define SHARP_LIBRARY_H

#include "../../../stdimports.h"

struct extern_lib {
    extern_lib()
    :
        name(),
        handle(nullptr)
    {
    }

#ifdef _WIN32
    HINSTANCE handle;
#else
    void* handle;
#endif
    string name;
};

extern recursive_mutex library_mutex;

#endif //SHARP_LIBRARY_H
