//
// Created by bknun on 2/24/2017.
//

#include "Monitor.h"

void Monitor::unlock() {
    if(status == monitor_free)
        return;

#ifdef WIN32_
    if( !ReleaseMutex(mutex) ) {
        // error
    } else
        status = monitor_free;
#endif
#ifdef POSIX_
    if(!pthread_mutex_unlock( &mutex ))
        status = monitor_free;
#endif
}

bool Monitor::lock() {
    bool success;
#ifdef WIN32_
    success = !WaitForSingleObject(
            mutex,    // handle to mutex
            INFINITE);  // no time-out interval
#endif
#ifdef POSIX_
    success = !pthread_mutex_lock( &count_mutex );
#endif
    if(success) status = monitor_busy;
    return success;
}
