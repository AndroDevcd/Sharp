//
// Created by BraxtonN on 2/11/2018.
//

#include "pThreadMutex.h"
#include "Thread.h"

int MUTEX_INIT(MUTEX *mutex)
{
    return pthread_mutex_init (mutex, NULL);
}

int MUTEX_LOCK(MUTEX *mutex)
{
    return pthread_mutex_lock( mutex );
}

int MUTEX_UNLOCK(MUTEX *mutex)
{
    return pthread_mutex_unlock( mutex );
}
