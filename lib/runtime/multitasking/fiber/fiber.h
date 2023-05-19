//
// Created by BNunnally on 9/5/2020.
//

#ifndef SHARP_FIBER_H
#define SHARP_FIBER_H

#include "../../../../stdimports.h"
#include "../../memory/sharp_object.h"
#include "../../memory/vm_stack.h"

class sharp_thread;
class sharp_function;
enum fiber_state {
    FIB_CREATED=0,
    FIB_RUNNING=1,
    FIB_SUSPENDED=2,
    FIB_KILLED=3
};

#define INITIAL_FRAME_SIZE 60 // ~1.2kb
#define FRAME_GROW_SIZE 150 // ~3kb

#define INITIAL_STACK_SIZE 200 // ~3.2kb
#define STACK_GROW_SIZE 512 // ~8kb

struct fib_mutex {
    Int id;
    sharp_object *o;
};

// cost per fiber: 4.4kb
// cost per 100k fibers: 440Mb
// cost per 1Mil fibers: 4.4Gb
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
    opcode_instr *rom, *pc;
    fiber_state state;
    int exitVal;
    object exceptionObject;
    object fiberObject;
    sharp_function *main;
    Int calls;
    Int callFramePtr;
    recursive_mutex mut;
    sharp_thread* attachedThread;
    sharp_thread* boundThread;
    stack_item* stack, *sp, *fp;
    sharp_function *current;
    frame *frames;
    uInt frameLimit;
    uInt frameSize;
    uInt stackSize;
    long double *registers;
    object *ptr;
    Int delayTime;
    bool wakeable;
    bool finished;
    fib_mutex *f_lock;
};

void init_struct(fiber*);

#endif //SHARP_FIBER_H
