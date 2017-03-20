//
// Created by bknun on 2/24/2017.
//

#include "Monitor.h"
#include "Thread.h"

void Monitor::unlock() {
    if(status == monitor_free)
        return;

    status = monitor_free;
    threadid = -1;
}


bool Monitor::acquire(int32_t spins) {
    if(Thread::self->id == threadid)
        return true;

    wait:
        _thread_wait_for_lock(spins);

    if(!do_lock())
    {
        if(spins == INDEFINITE)
            goto wait;
        else
            return false;
    } else {

        threadid = Thread::self->id;
        status = monitor_busy;
        return true;
    }
}

void Monitor::_thread_wait_for_lock(int32_t spins)
{
    int32_t retryCount = 1;
    while (!do_lock())
    {
        if(retryCount++ == spins)
        {
            if(spins == INDEFINITE)
            {
                retryCount = 1;
                continue;
            }

            return;
        }
    }
}
