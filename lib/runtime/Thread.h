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
#include "jit/architecture.h"
#include "../util/HashMap.h"

#define ILL_THREAD_ID -1
#define THREAD_MAP_SIZE 0x2000

#define INTERNAL_STACK_SIZE (MB_TO_BYTES(1) / sizeof(StackElement))
#define INTERNAL_STACK_MIN KB_TO_BYTES(1)
#define STACK_SIZE MB_TO_BYTES(1)
#define STACK_MIN KB_TO_BYTES(50)
#define STACK_OVERFLOW_BUF KB_TO_BYTES(10) // VERY LARGE OVERFLOW BUFFER FOR jit STACK OFERFLOW CATCHES

#define main_threadid 0x0
#define gc_threadid 0x1
#define jit_threadid 0x2

#ifdef BUILD_JIT
struct jit_context;
#endif

enum ThreadState {
    THREAD_CREATED      =0x000,
    THREAD_STARTED      =0x001,
    THREAD_RUNNING      =0x002,
    THREAD_SUSPENDED    =0x003,
    THREAD_KILLED       =0x004
};

enum ThreadPriority {
    THREAD_PRIORITY_LOW = 0x0001,
    THREAD_PRIORITY_NORM = 0X0004,
    THREAD_PRIORITY_HIGH = 0X0006
};

enum ThreadProcessingResult {
    RESULT_OK                      = 0x000,
    RESULT_ILL_THREAD_START        = 0x001,
    RESULT_THREAD_RUNNING          = 0x002,
    RESULT_THREAD_TERMINATED       = 0x003,
    RESULT_INVALID_STACK_SIZE      = 0x004,
    RESULT_THREAD_NOT_STARTED      = 0x005,
    RESULT_ILL_THREAD_JOIN         = 0x006,
    RESULT_THREAD_JOIN_FAILED      = 0x007,
    RESULT_ILL_THREAD_INTERRUPT    = 0x008,
    RESULT_THREAD_INTERRUPT_FAILED = 0x009,
    RESULT_ILL_THREAD_DESTROY      = 0x00a,
    RESULT_THREAD_CREATE_FAILED    = 0x00b,
    RESULT_NO_THREAD_ID            = 0x00c,
    RESULT_THREAD_DESTROY_FAILED   = 0x00d,
    RESULT_ILL_PRIORITY_SET        = 0x00e,
    RESULT_MAX_SPIN_GIVEUP         = 0x0ff
};

/**
 * Thread signals for events being handled in the threadding system
 *
 * Signals are send and read from at specific times and it makes
 * for a much faster processing to better handle signals as they arise
 */
enum tsig_t {
    tsig_empty = 0x000,
    tsig_except = 0x001,
    tsig_suspend = 0x002,
    tsig_kill = 0x004
};

class Thread {
public:
    Thread()
            :
            id(ILL_THREAD_ID),
            daemon(false),
            state(THREAD_CREATED),
            suspended(false),
            exited(false),
            terminated(false),
            priority(THREAD_PRIORITY_HIGH),
            name(),
            rand(new Random()),
            main(NULL),
            exitVal(0),
            signal(tsig_empty),
            throwable(),
            callStack(),
            dataStack(NULL),
            calls(0)
#ifdef BUILD_JIT
            ,jctx(NULL)
#endif

    {
        args.object = NULL;
        currentThread.object=NULL;
        exceptionObject.object=NULL;
        new (&mutex) std::mutex();
#ifdef WIN32_
        thread = NULL;
#endif
    }

    void init(string name, Int id, Method* main, bool daemon = false, bool initializeStack = false);
    static int32_t generateId();
    static void Startup();
    static void suspendSelf();
    static int start(int32_t, size_t);
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
    static bool validStackSize(size_t);
    static bool validInternalStackSize(size_t);
    static void suspendAllThreads();
    static void resumeAllThreads();

    static int startDaemon(
#ifdef WIN32_
            DWORD WINAPI
#endif
#ifdef POSIX_
    void*
#endif
    (*threadFunc)(void*), Thread* thread);


    static int32_t Create(int32_t, bool daemon);
    void Create(string, Method*);
    void CreateDaemon(string);
    void exit();
    void term();
    void exec();
    void setup();

    // easier to access for JIT
    long long calls;
    StackElement* dataStack,
            *sp, *fp;
    Method *current;
    Frame *callStack;
#ifdef BUILD_JIT
    jit_context *jctx;
#endif
    int64_t stackLimit;
    Cache cache, pc;
#ifdef SHARP_PROF_
    Profiler *tprof;
#endif
    /* tsig_t */ int signal;

    static uInt maxThreadId;
    static int32_t tid;
    static HashMap<Int, Thread*> threads;
    static std::mutex threadsMonitor;

    std::mutex mutex;
    int32_t id;
    int64_t stackSize;
    int64_t stbase;
    int64_t stfloor;
    int priority;
    bool daemon;
    bool terminated;
    unsigned int state;
    bool suspended;
    bool exited;
    native_string name;
    Method *main;
    int exitVal;
    Object currentThread, args;
    Object exceptionObject;
    Random* rand;

    Throwable throwable;
#ifdef WIN32_
    HANDLE thread;
#endif
#ifdef POSIX_
    pthread_t thread;
#endif

private:

    void wait();

    static int threadjoin(Thread*);
    static int unsuspendThread(Thread*);
    static void suspendThread(Thread*);
    static int interrupt(Thread*);

    static void pushThread(Thread *thread);
    static void popThread(Thread *thread);
    void releaseResources();
};

/**
 * Send a signal to shutdown and or sleep a thread and more
 * see tsig_t for more information.
 */
#define sendSignal(sig, sigt, enable) (sig ^= (-(unsigned long)enable ^ sig) & (1UL << sigt))
#define hasSignal(sig, sigt) ((sig >> sigt) & 1U)

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

extern unsigned long long irCount, overflow;
extern FinallyTable finallyTable;
extern thread_local short startAddress;
extern double exponent(int64_t n);
extern size_t threadStackSize;
extern size_t internalStackSize;

#endif //SHARP_THREAD_H
