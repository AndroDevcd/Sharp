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
    Int id;             // id of the fiber that holds the lock, -1 = free
    sharp_object *o;
    Int count;          // how many lock(o) blocks the holder is inside (see lock_object)
};

/**
 * One lock(x) { } block a fiber is inside of right now: which lock, and where
 * the block was entered (call depth and pc within that function). Kept so that
 * an exception leaving the block can give the lock back - see
 * release_locks_left_by_exception() in memory/sharp_object.cpp.
 */
struct held_lock {
    fib_mutex *mut;
    Int depth;          // task->callFramePtr when the lock was taken
    Int pc;             // pc of the LOCK instruction in that function
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

    // ---- picking a fiber up again after a context switch (virtual_machine.cpp,
    //      next_rebuild_frame) ----
    // When a thread continues a fiber, the native side of its call stack has to
    // be built again from the bottom: every JIT compiled function on the stack
    // gets its machine frame back. `rebuilding` is true while that walk is
    // under way and `rebuildDepth` is the call depth it has reached (0 = the
    // fiber's first function). The walk used to find its place by looking the
    // current FUNCTION up in the frame list, which finds the first occurrence:
    // with one function on the stack twice (recursion) it never got past it.
    Int rebuildDepth = 0;
    bool rebuilding = false;

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
    long long delayTime;        // scheduler time (sched_now_micros) it sleeps until, -1 = not sleeping
    bool wakeable;              // false while suspended by fiber.suspend()

    // ---- scheduler bookkeeping (guarded by task_mutex, see scheduler.h) ----
    fiber *readyNext;           // next fiber on the same ready queue
    bool queued;                // on a ready queue right now
    bool lockWaiting;           // listed as waiting for an object lock
    long long timerArmed;       // the delayTime a timer heap entry exists for, -1 = none
    bool finished;
    fib_mutex *f_lock;

    // the lock blocks this fiber is inside of, innermost last (a plain array:
    // fibers are allocated with malloc)
    held_lock *held;
    Int heldCount;
    Int heldCapacity;
};

void init_struct(fiber*);

#endif //SHARP_FIBER_H
