//
// Created by BraxtonN on 2/24/2017.
//

#ifndef SHARP_THREAD_H
#define SHARP_THREAD_H


#include "../../../stdimports.h"
#include "Monitor.h"
#include "../interp/FastStack.h"

class Method;

enum ThreadState {
    thread_init,
    thread_running,
    thread_suspend,
    thread_zombie,
    thread_killed
};

class Thread {
public:
    Thread()
    :
            id(-1),
            monitor(NULL),
            dameon(false),
            state(thread_killed),
            suspended(false),
            name(""),
            main(NULL),
            exitVal(-1),
            suspendPending(false)

    {
        threads = new list<Thread*>();
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


    void Create(string, int64_t, int64_t);

    thread_local
    static Thread* self;
    static int32_t tid;
    static list<Thread*>* threads;

    int32_t id;
    Monitor* monitor;
    bool dameon;
    ThreadState state;
    bool suspended;
    string name;
    Method* main;
    int exitVal;
    bool suspendPending;

    int32_t sp, ip;
    FastStack stack;
#ifdef WIN32_
    HANDLE thread;
#endif
#ifdef POSIX_
    pthread_t thread;
#endif

private:

    void wait();
    void term();

    static int threadjoin(Thread*);
    static int unsuspendThread(Thread*);
    static void suspendThread(Thread*);
};

#define ZombieMax 0x7e
#define main_threadid 0x0


#endif //SHARP_THREAD_H
