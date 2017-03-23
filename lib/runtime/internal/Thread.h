//
// Created by BraxtonN on 2/24/2017.
//

#ifndef SHARP_THREAD_H
#define SHARP_THREAD_H


#include "../../../stdimports.h"
#include "Monitor.h"
#include "../interp/FastStack.h"
#include "../oo/ClassObject.h"
#include "../oo/Exception.h"
#include "../interp/CallStack.h"

#define MAX_THREADS 0x40fe

class Method;

enum ThreadState {
    thread_init,
    thread_running,
    thread_suspend,
    thread_killed
};

class Thread {
public:
    Thread()
    :
            id(-1),
            monitor(),
            dameon(false),
            state(thread_killed),
            suspended(false),
            name(""),
            main(NULL),
            exitVal(1),
            suspendPending(false),
            exceptionThrown(false),
            throwable()

    {

#ifdef WIN32_
        thread = NULL;
#endif
    }

    static void Startup();
    static void suspendSelf();
    static int start(int32_t);
    static int interrupt(int32_t);
    static int join(int32_t);
    static int waitForThread(Thread* thread);
    static Thread* getThread(int32_t);
    static void waitForThreadSuspend(Thread* thread);
    static int unsuspendThread(int32_t);
    static void suspendThread(int32_t);
    static void killAll();
    static void shutdown();


    void Create(string, ClassObject*, int64_t);
    void Create(string);
    void exit();

    thread_local
    static Thread* self;
    static int32_t tid;

    static Thread** threads;
    static unsigned int tp;

    int32_t id;
    Monitor monitor;
    bool dameon;
    ThreadState state;
    bool suspended;
    nString name;
    Method* main;
    int exitVal;
    bool suspendPending;
    bool exceptionThrown;

    FastStack stack;
    CallStack cstack;
    Throwable throwable;
#ifdef WIN32_
    HANDLE thread;
#endif
#ifdef POSIX_
    pthread_t thread;
#endif

    void term();

private:

    void wait();

    static int threadjoin(Thread*);
    static int unsuspendThread(Thread*);
    static void suspendThread(Thread*);
    static int interrupt(Thread*);

    void push_thread(Thread *thread) const;
};

#define ZombieMax 0x7e
#define main_threadid 0x0


#endif //SHARP_THREAD_H
