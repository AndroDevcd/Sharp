//
// Created by BraxtonN on 2/11/2018.
//

#include "Mutex.h"
#include "Thread.h"

void Mutex::release() {
    if(status == monitor_free)
        return;

    status = monitor_free;
    threadid = -1;
}


bool Mutex::acquire(int32_t spins) {
    if(thread_self == NULL) return true;

    if(thread_self->id == threadid)
        return true;

    wait:
    _thread_wait_for_lock(spins);

    if(locked())
    {
        if(spins == INDEFINITE)
            goto wait;
        else
            return false;
    } else {

        threadid = thread_self->id;
        status = monitor_busy;
        return true;
    }
}

#pragma GCC push_options
#pragma GCC optimize ("O0")
void Mutex::_thread_wait_for_lock(int32_t spins)
{
    int32_t retryCount = 1;

    while (locked())
    {
        if(retryCount++ == spins)
        {
            if(spins == INDEFINITE)
            {
                __os_sleep(1);
                retryCount = 1;
                continue;
            }

            return;
        }
    }
}
#pragma GCC pop_options