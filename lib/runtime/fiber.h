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
    FIB_CREATED,
    FIB_RUNNING,
    FIB_SUSPENDED,
    FIB_KILLED
};

class fiber {
public:
    static fiber* makeFiber(string name, Method* main);
    static fiber* getFiber(uInt id);
    static fiber* nextFiber();
    static void disposeFiber(uInt id);

    void setState(Thread* thread, fiber_state, Int delay = -1);
    void setWakeable(bool enable);
    void free();

    static uInt fibId;
    uInt id;
    native_string name;
    Int stackLimit;
    Cache cache, pc;
    fiber_state state;
    int exitVal;
    Object exceptionObject;
    Method *main;
    Int calls;
    StackElement* dataStack, *sp, *fp;
    Method *current;
    Frame *callStack;
    double *registers;
    Object *ptr;
    Int delayTime; // -1 for full suspension >= 0 for timed suspension
    recursive_mutex mutex;
    bool wakeable;
};

#endif //SHARP_FIBER_H
