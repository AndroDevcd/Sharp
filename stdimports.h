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
#include <thread>
#include <limits>
#include <cmath>
#include <mutex>
#include "lib/runtime/architecture.h"
#include "lib/grammar/platform.h"

using namespace std;

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
#define WIN32_
#else
#define POSIX_
#endif

#ifdef WIN32_

#define __os_yield() std::this_thread::yield();
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
#include <stdarg.h>
#include <dlfcn.h>

#define POSIX_USEC_INTERVAL 1000

#define MUTEX pthread_mutex_t
#define __os_yield() sched_yield();
#endif

//#define GOD_MODE

#ifdef GOD_MODE
#define SHARP_PROF_
#endif
#define COROUTINE_DEBUGGING

#ifdef SHARP_PROF_
#define PROFILER_NAME "tanto"
#endif

#ifndef DEBUGGING
#define DEBUGGING
#endif

#if _ARCH_BITS == 32
typedef int32_t Int;
typedef uint32_t uInt;
#define _INT_MAX INT32_MAX
#else
typedef int64_t Int;
typedef uint64_t uInt;
#define _INT_MAX INT64_MAX
#endif

typedef void (*fptr)(void *);

void* __malloc(uInt bytes);
void* __calloc(uInt n, uInt bytes);
void* __realloc(void *ptr, uInt bytes, uInt);
void setupSigHandler();

template <class T>
T* mallocAndNew() {
    void * data = __malloc(sizeof(T));
    return new (data) T();
}

template <class T>
T* mallocAndNew(uInt quantity) {
    void * data = __malloc(sizeof(T) * quantity);
    T * p = data;

    p = data;
    for(uInt i = 0; i < quantity; i++) {
        new (p) T();
        p++;
    }

    return data;
}

#define CXX11_INLINE inline


#endif //SHARP_STDIMPORTS_H
