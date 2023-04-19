//
// Created by bknun on 9/16/2022.
//

#ifndef SHARP_SHARP_THREAD_H
#define SHARP_SHARP_THREAD_H

#include "handshake.h"
#include "../scheduler/scheduler.h"
#include "../../memory/sharp_object.h"
#include "../../../grammar/backend/types/sharp_function.h"

#define ILL_THREAD_ID -1
#define THREAD_MAP_SIZE 0x2000
#define REGISTER_SIZE 12

#define main_threadid 0x0
#define gc_threadid 0x1

/**
 * Send a signal to shutdown and or sleep a thread and more
 * see tsig_t for more information.
 */
#define sendSignal(sig, sigt, enable) (sig ^= (-(unsigned long)enable ^ sig) & (1UL << sigt))
#define hasSignal(sig, sigt) ((sig >> sigt) & 1U)

extern thread_local sharp_thread* thread_self;

struct sharp_thread {
    handshake hs;
    sched_task *scht;
    atomic<Int> boundFibers;
    recursive_mutex mut;
    Int id;
    Int stackSize;
    Int stbase;
    Int stfloor;
    Int signal;
    int priority;
    bool daemon;
    bool terminated;
    unsigned int state;
    bool suspended;
    bool exited;
    bool marked;
    std::string name;
    object currentThread, args;
    sharp_function* mainMethod;
    uInt lastRanMicros;
    int execution_state;
#ifdef COROUTINE_DEBUGGING
    Int timeSleeping, switched, skipped,
            actualSleepTime, contextSwitchTime, timeLocking;
#endif
#ifdef WIN32_
    HANDLE thread;
#endif
#ifdef POSIX_
    pthread_t thread;
#endif
};

void init_struct(sharp_thread *thread);


#endif //SHARP_SHARP_THREAD_H
