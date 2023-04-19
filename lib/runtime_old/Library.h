//
// Created by BNunnally on 6/27/2020.
//

#ifndef SHARP_LIBRARY_H
#define SHARP_LIBRARY_H

#include "../../stdimports.h"

class Library {
public:
    Library()
    : name("")
    {
        init();
    }

    void init() {
        handle = NULL;
        name.clear();
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
