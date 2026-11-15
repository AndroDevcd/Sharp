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
    fiber *task;
    atomic<Int> boundFibers;
    recursive_mutex mut;
    Int id;
    Int stackSize;
    // The lowest address of this thread's MACHINE stack that code may still
    // use (set when the thread starts, vm_thread_entry). Compiled code nests one
    // machine frame per Sharp call it makes; before it goes one deeper it tests
    // the machine stack pointer against this (jit/x64/opcode/direct_calls.cpp)
    // and prepare_method() raises stack_overflow_exception when it is passed.
    // 0: not a thread that runs Sharp code (nothing is tested).
    uintptr_t nativeStackFloor;
    Int signal;
    Int nativeCalls;
    int priority;
    bool daemon;
    bool terminated;
    unsigned int state;
    bool suspended;
    bool exited;
    bool marked;
    // 1 = this thread is not touching objects right now (blocked, not started,
    // finished, or not a thread that runs Sharp code at all) and the garbage
    // collector may work without waiting for it. Changed only through
    // tgc_enter_safe / tgc_leave_safe (memory/tracing_collector.h).
    int gcSafe;
    // How many callers are using this struct right now through hold_thread()
    // (starting or joining the thread). The scheduler's monitor frees the struct
    // of a finished thread, and a short thread finishes while its starter is
    // still waiting for it: the struct is only freed when this is 0.
    int users;
    std::string name;
    object currentThread, args;
    uInt lastRanMicros;         // scheduler time (sched_now_micros) the current fiber started running

    // ---- scheduler bookkeeping (guarded by task_mutex, see scheduler.h) ----
    fiber *readyHead, *readyTail;   // ready fibers bound to this thread (FIFO)
    bool parked;                    // asleep in sched_next_task() waiting for a fiber
    uInt pickCount;                 // fibers picked so far, used to mix in unbound fibers fairly
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
