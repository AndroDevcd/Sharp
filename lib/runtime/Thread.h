//
// Created by BraxtonN on 2/12/2018.
//

#ifndef SHARP_THREAD_H
#define SHARP_THREAD_H

#include "../../stdimports.h"
#include "oo/ClassObject.h"
#include "oo/Exception.h"
#include "oo/Method.h"

#define MAX_THREADS 0xffba

#define STACK_SIZE 0xcfba

enum ThreadState {
    THREAD_CREATED      =0x000,
    THREAD_RUNNING      =0x001,
    THREAD_SUSPENDED    =0x002,
    THREAD_KILLED       =0x003
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
            main(NULL),
            exitVal(1),
            suspendPending(false),
            exceptionThrown(false),
            throwable(),
            callStack(),
            dataStack(NULL)

    {
    #ifdef WIN32_
            mutex.initalize();
    #endif
    #ifdef POSIX_
            mutex = std::mutex();
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

    static int32_t tid;
    static List<Thread*> threads;
#ifdef WIN32_
    static recursive_mutex threadsMonitor;
    recursive_mutex mutex;
#endif
#ifdef POSIX_
    std::mutex threadsMonitor;
    std::mutex mutex;
#endif
    static bool isAllThreadsSuspended;

    int32_t id;
    bool daemon;
    bool terminated;
    unsigned int state;
    unsigned int signal;
    bool suspended;
    bool exited;
    native_string name;
    Method *main;
    int exitVal;
    bool suspendPending;
    bool exceptionThrown;

    uint64_t pc;
    Method *current;
    List<Frame> callStack;
    StackElement* dataStack;
    unsigned long stack_lmt;
    Cache cache;
    int64_t cacheSize;
    Throwable throwable;
#ifdef WIN32_
    HANDLE thread;
#endif
#ifdef POSIX_
    pthread_t thread;
#endif

// @Remove
    void dbg();
    void term();

    static void suspendAllThreads();

    static void resumeAllThreads();

    void exec();

    void interrupt();

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

#define main_threadid 0x0

extern FinallyTable finallyTable;
extern short int startAddress;

#endif //SHARP_THREAD_H
