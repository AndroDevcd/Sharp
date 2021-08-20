//
// Created by BNunnally on 9/5/2020.
//

#ifndef SHARP_FIBER_H
#define SHARP_FIBER_H

#include "../../stdimports.h"
#include "profiler.h"

class Thread;
enum fiber_state {
    FIB_CREATED=0,
    FIB_RUNNING=1,
    FIB_SUSPENDED=2,
    FIB_KILLED=3
};

#define INITIAL_FRAME_SIZE 100 // ~2kb
#define FRAME_GROW_SIZE 150 // ~3kb

#define INITIAL_STACK_SIZE 250 // ~4kb
#define STACK_GROW_SIZE 512 // ~8kb

class fiber {
public:
    fiber()
    :
        name("")
    {}

    void growFrame();
    void growStack(Int requiredSize = 0);
    void free();

public:

    uInt id;
    std::string name;
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
    mutex_t *acquiringMut;
};

#endif //SHARP_FIBER_H
