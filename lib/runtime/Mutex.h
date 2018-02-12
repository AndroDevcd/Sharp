//
// Created by BraxtonN on 2/11/2018.
//

#ifndef SHARP_MUTEX_H
#define SHARP_MUTEX_H


#include "../../stdimports.h"

#define INDEFINITE 0

enum LockStatus {
    mutex_busy=0x1,
    mutex_free=0x0
};

class Mutex {
public:
    Mutex()
    :
            threadid(-1),
            status(mutex_free)
    {
    }

    ~Mutex()
    {
        release();
    }

    CXX11_INLINE
    bool locked() {
        return status == mutex_busy;
    }

    void _thread_wait_for_lock(int32_t);
    bool acquire(int32_t spins = INDEFINITE);
    void release();

    int32_t  threadid;
    LockStatus status;
};


#endif //SHARP_MUTEX_H
