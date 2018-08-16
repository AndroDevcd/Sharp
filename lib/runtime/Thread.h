//
// Created by BraxtonN on 2/12/2018.
//

#ifndef SHARP_THREAD_H
#define SHARP_THREAD_H

#include "../../stdimports.h"
#include "oo/ClassObject.h"
#include "oo/Exception.h"
#include "oo/Method.h"
#include "profiler.h"
#include "../Modules/std/Random.h"

#define MAX_THREADS 0xffba

#define STACK_SIZE 0xcfba

#define main_threadid 0x0
#define gc_threadid 0x1

enum ThreadState {
    THREAD_CREATED      =0x000,
    THREAD_RUNNING      =0x001,
    THREAD_SUSPENDED    =0x002,
    THREAD_KILLED       =0x003
};

enum ThreadPriority {
    THREAD_PRIORITY_LOW = 0x0001,
    THREAD_PRIORITY_NORM = 0X0004,
    THREAD_PRIORITY_HIGH = 0X0006
};

class Thread {
public:
    Thread()
            :
            id(-1),
            daemon(false),
            state(THREAD_KILLED),
            suspended(false),
            name(""),
            rand(NULL),
            main(NULL),
            exitVal(1),
            suspendPending(false),
            exceptionThrown(false),
            throwable(),
            callStack(),
            dataStack(NULL)

    {
#ifdef SHARP_PROF_
        tprof = NULL;
#endif
    #ifdef WIN32_
        new (&mutex) std::mutex();
    #endif
    #ifdef POSIX_
        new (&mutex) std::mutex();
    #endif

#ifdef WIN32_
        thread = NULL;
#endif
    }

    static void Startup();
    static void suspendSelf();
    static int start(int32_t);
    static int destroy(int64_t);
    static int interrupt(int32_t);
    static int join(int32_t);
    static Thread* getThread(int32_t);
    static void waitForThreadSuspend(Thread* thread);
    static void waitForThreadExit(Thread* thread);
    static void terminateAndWaitForThreadExit(Thread* thread);
    static int waitForThread(Thread *thread);
    static int setPriority(int32_t, int);
    static int setPriority(Thread*, int);
    static void killAll();
    static void shutdown();

    static int startDaemon(
#ifdef WIN32_
            DWORD WINAPI
#endif
#ifdef POSIX_
    void*
#endif
    (*threadFunc)(void*), Thread* thread);


    static int32_t Create(int32_t, unsigned long);
    void Create(string);
    void CreateDaemon(string);
    void exit();

    void initJitCtx();

    // easier to acces for JIT
    unsigned long calls;
    StackElement* dataStack,
            *sp, *fp;
    Method *current;
    Frame *callStack;
    unsigned long stack_lmt;
    Cache cache, pc;
#ifdef SHARP_PROF_
    Profiler *tprof;
#endif

    static int32_t tid;
    static List<Thread*> threads;
#ifdef WIN32_
    static std::mutex threadsMonitor;
    std::mutex mutex;
#endif
#ifdef POSIX_
    static std::mutex threadsMonitor;
    std::mutex mutex;
#endif
    static bool isAllThreadsSuspended;

    int32_t id;
    int priority;
    bool daemon;
    bool terminated;
    unsigned int state;
    unsigned int starting;
    bool suspended;
    bool exited;
    native_string name;
    Method *main;
    int exitVal;
    bool suspendPending;
    bool exceptionThrown;
    Object currentThread, args;
    Random* rand;

    Throwable throwable;
#ifdef WIN32_
    HANDLE thread;
#endif
#ifdef POSIX_
    pthread_t thread;
#endif

// @Remove
    void term();

    static void suspendAllThreads();

    static void resumeAllThreads();

    void exec();

    void interrupt();

    void setup();
private:

    void wait();

    static int threadjoin(Thread*);
    static int unsuspendThread(Thread*);
    static void suspendThread(Thread*);
    static int interrupt(Thread*);

    static void pushThread(Thread *thread);
    static void popThread(Thread *thread);
};

extern thread_local Thread* thread_self;
extern thread_local double registers[12];

#define _64EBX registers[i64ebx]
#define _64ADX registers[i64adx]
#define _64ECX registers[i64ecx]
#define _64EGX registers[i64egx]
#define _64CMT registers[i64cmt]
#define _64BMR registers[i64bmr]

#define PC(thread_self) \
    (thread_self->pc-thread_self->cache)

extern FinallyTable finallyTable;
extern short int startAddress;

#endif //SHARP_THREAD_H
