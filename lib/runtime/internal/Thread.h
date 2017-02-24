//
// Created by BraxtonN on 2/24/2017.
//

#ifndef SHARP_THREAD_H
#define SHARP_THREAD_H


#include "../../../stdimports.h"

class Thread {
public:

    static void Startup();

    uint32_t id;
    HANDLE* pthread;
};


#endif //SHARP_THREAD_H
