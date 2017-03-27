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
    thread_init=0x0,
    thread_running=0x1,
    thread_suspend=0x2,
    thread_killed=0x3
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
    static Thread* getThread(int32_t);
    static void waitForThreadSuspend(Thread* thread);
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


    void Create(string, ClassObject*, int64_t);
    void Create(string);
    void CreateDaemon(string);
    void exit();

    static int32_t tid;

    static Thread** threads;
    static unsigned int tp;

    int32_t id;
    Monitor monitor;
    bool dameon;
    int state;
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

    static void suspendAllThreads();

    static void resumeAllThreads();

private:

    void wait();

    static int threadjoin(Thread*);
    static int unsuspendThread(Thread*);
    static void suspendThread(Thread*);
    static int interrupt(Thread*);

    void push_thread(Thread *thread) const;

};

extern thread_local Thread* thread_self;

#define ZombieMax 0x7e
#define main_threadid 0x0


#endif //SHARP_THREAD_H
