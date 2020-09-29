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

#define INITIAL_FRAME_SIZE 500 // ~16kb
#define FRAME_GROW_SIZE 250 // ~8kb

class fiber {
public:

    static fiber* makeFiber(native_string &name, Method* main);
    static fiber* getFiber(uInt id);
    static fiber* nextFiber(fiber *startingFiber, Thread* thread);
    static int suspend(uInt id);
    static int unsuspend(uInt id);
    static int kill(uInt id);
    static Int boundFiberCount(Thread *thread);
    static void killBoundFibers(Thread *thread);

    void growFrame();
    int getState();
    void setState(Thread* thread, fiber_state, Int delay = -1);
    void setWakeable(bool enable);
    void free();
    Thread *getAttachedThread();
    Thread *getBoundThread();
    void setAttachedThread(Thread *thread);
    void delay(Int time);
    int bind(Thread *thread);
    static void disposeFiber(fiber *);

public:

    static uInt fibId;
    uInt id;
    native_string name;
    Int stackLimit;
    Cache cache, pc;
    fiber_state state;
    int exitVal;
    Object exceptionObject;
    Object fiberObject;
    Method *main;
    Int calls;
    Thread* attachedThread;
    Thread* boundThread;
    StackElement* dataStack, *sp, *fp;
    Method *current;
    Frame *callStack;
    uInt frameLimit;
    uInt frameSize;
    double *registers;
    Object *ptr;
    Int delayTime; // -1 for full suspension >= 0 for timed suspension
    bool wakeable;
    bool finished;
    bool locking;
};

#endif //SHARP_FIBER_H
