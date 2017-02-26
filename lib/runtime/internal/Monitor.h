//
// Created by bknun on 2/24/2017.
//

#ifndef SHARP_MONITOR_H
#define SHARP_MONITOR_H

#include "../../../stdimports.h"

enum LockStatus {
    monitor_busy,
    monitor_free
};

class Monitor {
public:
    Monitor()
    :
            threadid(-1),
            status(monitor_free)
    {
        #ifdef WIN32_
        mutex = CreateMutex(
                NULL,              // default security attributes
                FALSE,             // initially not owned
                NULL);
        #endif
        #ifdef POSIX_
        //pthread_mutex_init ( &mutex, NULL);
        #endif
    }

    bool lock();
    void unlock();

    int32_t  threadid;
    LockStatus status;

#ifdef WIN32_
    HANDLE mutex;
#endif
#ifdef POSIX_
    pthread_mutex_t mutex;
#endif
};


#endif //SHARP_MONITOR_H
