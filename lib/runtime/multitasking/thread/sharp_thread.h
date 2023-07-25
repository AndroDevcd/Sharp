//
// Created by bknun on 9/16/2022.
//

#ifndef SHARP_SHARP_THREAD_H
#define SHARP_SHARP_THREAD_H

#include "../../../../stdimports.h"
#include "../scheduler/scheduler.h"
#include "../../memory/sharp_object.h"
#include "notification_helper.h"
#include "../fiber/fiber.h"

#define ILL_THREAD_ID -1
#define THREAD_MAP_SIZE 0x2000
#define REGISTER_SIZE 12

#define main_threadid 0x0
#define gc_threadid 0x1
#define idle_threadid 0x2

/**
 * Send a signal to shutdown and or sleep a thread and more
 * see tsig_t for more information.
 */
#define sendSignal(sig, sigt, enable) (sig ^= (-(unsigned long)enable ^ sig) & (1UL << sigt))
#define hasSignal(sig, sigt) ((sig >> sigt) & 1U)

extern thread_local sharp_thread* thread_self;

struct sharp_thread {
    atomic<fiber*> queue;
    notification queueNotification;
    sched_task *last_sched;
    fiber *task;
    atomic<Int> boundFibers;
    recursive_mutex mut;
    Int id;
    Int stackSize;
    Int signal;
    Int nativeCalls;
    int priority;
    bool daemon;
    bool terminated;
    unsigned int state;
    bool suspended;
    bool exited;
    bool marked;
    std::string name;
    object currentThread, args;
    uInt lastRanMicros;
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
void free_struct(sharp_thread *thread);


#endif //SHARP_SHARP_THREAD_H
