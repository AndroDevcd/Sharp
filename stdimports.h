//
// Created by bknun on 1/6/2017.
//

#ifndef SHARP_STDIMPORTS_H
#define SHARP_STDIMPORTS_H

#include <iostream>
#include <cstdlib>
#include <sstream>
#include <list>
#include <stdexcept>
#include <string>
#include <cstdio>
#include <cstring>


using namespace std;

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
#define WIN32_
#include "lib/util/mingw.mutex.h"
#else
#define POSIX_
#endif

#ifdef WIN32_

#define __os_yield() _mm_pause
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#ifndef __wtypes_h__
#include <wtypes.h>
#endif

#ifndef __WINDEF_
#include <windef.h>
#endif
#endif
#ifdef POSIX_
    #include <pthread.h>
    #include <unistd.h>
    #include <sys/time.h>

    #define POSIX_USEC_INTERVAL 1000

    #define MUTEX pthread_mutex_t
    #define __os_yield() pthread_yield();
#endif

#define SHARP_PROF_

#ifndef DEBUGGING
#define DEBUGGING
#endif

struct Sharp {
    /**
     * Jan 13, 2017 Initial build release of sharp 0.1.3, this contains
     * all the base level support for running sharp. Sharp was
     * developed to support both windows and linux operating systems
     */
    int BASE  = 1;

    /**
     * Jan 18, 2018 Build release of Sharp 0.2.0, this contains all the base level
     * support for sharp as well as major performance enhancments and improvements
     * throughout the platform.
     */
    int ALPHA = 2;
};

extern Sharp versions;

void* __malloc(size_t bytes);
void* __calloc(size_t n, size_t bytes);
void* __realloc(void *ptr, size_t bytes);
void __os_sleep(int64_t);

#define CXX11_INLINE inline


#endif //SHARP_STDIMPORTS_H
