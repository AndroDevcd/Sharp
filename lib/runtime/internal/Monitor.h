//
// Created by bknun on 2/24/2017.
//

#ifndef SHARP_MONITOR_H
#define SHARP_MONITOR_H

#include "../../../stdimports.h"

#define INDEFINITE 0

enum LockStatus {
    monitor_busy=0x1,
    monitor_free=0x0
};

class Monitor {
public:
    Monitor()
    :
            threadid(-1),
            status(monitor_free)
    {
    }

    ~Monitor()
    {
        unlock();
    }

    CXX11_INLINE
    bool do_lock() {
        return status == monitor_free;
    }

    void _thread_wait_for_lock(int32_t);
    bool acquire(int32_t spins = INDEFINITE);
    void unlock();

    int32_t  threadid;
    LockStatus status;
};


#endif //SHARP_MONITOR_H
