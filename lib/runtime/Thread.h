//
// Created by BraxtonN on 2/12/2018.
//

#ifndef SHARP_THREAD_H
#define SHARP_THREAD_H

#include "../../stdimports.h"
#include "symbols/ClassObject.h"
#include "symbols/Exception.h"
#include "symbols/Method.h"
#include "profiler.h"
#include "../Modules/std/Random.h"
#include "architecture.h"
#include "../util/HashMap.h"
#include "ThreadStates.h"
#include "fiber.h"

#define ILL_THREAD_ID -1
#define THREAD_MAP_SIZE 0x2000
#define REGISTER_SIZE 12

#define INTERNAL_STACK_SIZE (KB_TO_BYTES(64) / sizeof(StackElement))
#define INTERNAL_STACK_MIN KB_TO_BYTES(4)
#define STACK_SIZE MB_TO_BYTES(1)
#define STACK_MIN KB_TO_BYTES(50)
#define STACK_OVERFLOW_BUF KB_TO_BYTES(10) // VERY LARGE OVERFLOW BUFFER FOR jit STACK OFERFLOW CATCHES

#define main_threadid 0x0
#define gc_threadid 0x1
#define idle_sched_threadid 0x2

#ifdef BUILD_JIT
struct jit_context;
#endif

class Thread {
public:
    Thread()
        : name("")
    {
        init();
    }

    void init() {
        id = ILL_THREAD_ID;
        daemon = false;
        state = THREAD_CREATED;
        suspended = false;
        exited = false;
        terminated = false;
        priority = THREAD_PRIORITY_NORM;
        this_fiber = NULL;
        signal = tsig_empty;
        contextSwitching=false;
        marked=false;
        waiting=false;
        lastRanMicros=0;
        args.object = NULL;
        currentThread.object=NULL;
        newTask = NULL;
        new (&mutex) recursive_mutex();
        boundFibers=0;
#ifdef COROUTINE_DEBUGGING
        timeSleeping=0;
        actualSleepTime=0;
        contextSwitchTime=0;
        switched=0;
        skipped=0;
        timeLocking=0;
#endif
#ifdef WIN32_
        thread = NULL;
#endif
    }

    void free();
    void exec();

    /* tsig_t */ int signal;

    static uInt maxThreadId;
    static uInt tid;

    uInt boundFibers;
    recursive_mutex mutex; // tODO: add boundOnly : bool to only run bound tasks only
    int32_t id;
    Int stackSize;
    Int stbase;
    Int stfloor;
    int priority;
    bool daemon;
    bool terminated;
    unsigned int state;
    bool suspended;
    bool exited;
    bool marked;
    std::string name;
    Object currentThread, args;
    fiber *this_fiber;
    Method* mainMethod;
    uInt lastRanMicros;
    bool contextSwitching;
    bool waiting;
    fiber *newTask;

#ifdef SHARP_PROF_
    Profiler *tprof;
#endif
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

/**
 * Send a signal to shutdown and or sleep a thread and more
 * see tsig_t for more information.
 */
#define sendSignal(sig, sigt, enable) (sig ^= (-(unsigned long)enable ^ sig) & (1UL << sigt))
#define hasSignal(sig, sigt) ((sig >> sigt) & 1U)

extern thread_local Thread* thread_self;
extern thread_local double *registers;

#define _64EBX registers[EBX]
#define _64ADX registers[ADX]
#define _64ECX registers[ECX]
#define _64EGX registers[EGX]
#define _64CMT registers[CMT]
#define _64BMR registers[BMR]

#define PC(thread_self) \
    (thread_self->this_fiber->pc-thread_self->this_fiber->cache)

extern unsigned long irCount, overflow;
extern size_t threadStackSize;
extern size_t internalStackSize;
extern string dataTypeToString(DataType varType, bool isArray);
extern string getVarCastExceptionMsg(DataType varType, bool isArray, SharpObject *obj);

#endif //SHARP_THREAD_H
