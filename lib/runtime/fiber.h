//
// Created by BNunnally on 9/5/2020.
//

#ifndef SHARP_FIBER_H
#define SHARP_FIBER_H

#include "../../stdimports.h"
#include "symbols/string.h"
#include "profiler.h"

class Thread;
enum fiber_state {
    FIB_CREATED=0,
    FIB_RUNNING=1,
    FIB_SUSPENDED=2,
    FIB_KILLED=3
};

#define INITIAL_FRAME_SIZE 50 // ~2kb
#define FRAME_GROW_SIZE 150 // ~6kb

#define INITIAL_STACK_SIZE 250 // ~4kb
#define STACK_GROW_SIZE 512 // ~8kb

class fiber {
public:

    static fiber* makeFiber(native_string &name, Method* main);
    static fiber* getFiber(uInt id);
    static fiber* nextFiber(Int startingIndex, Thread* thread);
    static int suspend(uInt id);
    static int unsuspend(uInt id);
    static int kill(uInt id);
    static Int boundFiberCount(Thread *thread);
    static void killBoundFibers(Thread *thread);

    void growFrame();
    void growStack(Int requiredSize = 0);
    int getState();
    void setState(Thread* thread, fiber_state, Int delay = -1);
    void setWakeable(bool enable);
    void free();
    Thread *getAttachedThread();
    Thread *getBoundThread();
    void setAttachedThread(Thread *thread);
    void delay(Int time, bool incPc = true);
    int bind(Thread *thread);
    bool safeStart(Thread *thread);
    static void disposeFibers();

public:

    uInt id;
    uInt itemIndex;
    native_string name;
    Int stackLimit;
    Cache cache, pc;
    fiber_state state;
    int exitVal;
    Object exceptionObject;
    Object fiberObject;
    Method *main;
    Int calls;
    recursive_mutex mut;
    Thread* attachedThread;
    Thread* boundThread;
    StackElement* dataStack, *sp, *fp;
    Method *current;
    Frame *callStack;
    uInt frameLimit;
    uInt frameSize;
    uInt stackSize;
    double *registers;
    Object *ptr;
    Int delayTime;
    bool wakeable;
    bool finished;
    bool locking;
    bool marked;
    Int passed;
};

extern atomic<Int> unBoundFibers;

#endif //SHARP_FIBER_H
