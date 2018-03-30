//
// Created by BraxtonN on 2/11/2018.
//

#ifndef SHARP_MUTEX_H
#define SHARP_MUTEX_H

#include <chrono>
#include <thread>
#include <mutex>
#include "../../stdimports.h"

//Headers
#if defined(POSIX_)
    #include <pthread.h>
#elif defined(WIN32_)
    #include <windows.h>
    #include <process.h>
#endif

//Data types
#if defined(POSIX_)
    #define MUTEX pthread_mutex_t
#elif defined(WIN32_)
    #define MUTEX HANDLE
#endif

#if defined(POSIX_)
    #define MU_LOCK pthread_mutex_lock( &handle );
#elif defined(WIN32_)
    #define MU_LOCK(handle) WaitForSingleObject(&handle, INFINITE);
#endif

#if defined(POSIX_)
    #define MU_ULOCK pthread_mutex_unlock( &handle );
#elif defined(WIN32_)
    #define MU_ULOCK(handle) ReleaseMutex(&handle);
#endif \

//Functions
int MUTEX_INIT(MUTEX *mutex);

#define MUTEX_LOCK(handle) \
    if(handle != NULL) { \
        MU_LOCK(handle) \
    }

#define MUTEX_UNLOCK(handle) \
    if(handle != NULL) { \
        MU_ULOCK(handle) \
    }


#endif //SHARP_MUTEX_H
