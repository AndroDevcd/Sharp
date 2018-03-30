//
// Created by BraxtonN on 2/11/2018.
//

#include "Mutex.h"
#include "Thread.h"

int MUTEX_INIT(MUTEX *mutex)
{
#if defined(POSIX_)
    return pthread_mutex_init (mutex, NULL);;
#elif defined(WIN32_)
    *mutex = CreateMutex(0, FALSE, 0);;
        return (*mutex==0);
#endif
}
