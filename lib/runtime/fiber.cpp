//
// Created by BNunnally on 9/5/2020.
//
#include "fiber.h"
#include "Thread.h"
#include "VirtualMachine.h"

uInt fiber::fibId=0;
recursive_mutex fiberMutex;
_List<fiber*> fibers;
uInt lastFiberIndex = 0;

bool find_fiber(void *arg1, fiber* fib) {
    return *((uInt*)arg1) == fib->id;
}

bool find_next_fiber(void *arg1, fiber* fib) {
    return fib->state == FIB_SUSPENDED && fib->delayTime >= 0 && *((Int*)arg1) != fib->id;
}

fiber* fiber::makeFiber(string name, Method* main) {
    GUARD(fiberMutex)
    // todo: check if space available before allocation
   fiber *fib = (fiber*)malloc(sizeof(fiber));
   fib->id = fibId++;
   fib->name.init();
   fib->name = name;
   fib->main = main;
   fib->cache=NULL;
   fib->pc=NULL;
   fib->pc=NULL;
   fib->state=FIB_CREATED;
   fib->exitVal=0;
   fib->wakeable=true;
   fib->exceptionObject.object=NULL;
   fib->calls=-1;
   new (&fib->mutex) std::recursive_mutex();
   fib->stackLimit = internalStackSize;
   fib->registers = (double*)calloc(REGISTER_SIZE, sizeof(double));
   fib->dataStack = (StackElement *) calloc(internalStackSize, sizeof(StackElement));
   fib->callStack = (Frame*)__calloc(internalStackSize, sizeof(Frame));
   fib->fp = &fib->dataStack[vm.manifest.threadLocals];
   fib->sp = (&fib->dataStack[vm.manifest.threadLocals]) - 1;

    gc.addMemory(sizeof(Frame) * internalStackSize);
    gc.addMemory(sizeof(StackElement) * internalStackSize);
    gc.addMemory(sizeof(double) * REGISTER_SIZE);
    fibers.add(fib);
    return fib;
}

fiber* fiber::getFiber(uInt id) {
    GUARD(fiberMutex)
    Int pos = fibers.indexof(find_fiber, &id);
    if(pos >= 0) {
        return fibers.at(pos);
    }

    return NULL;
}

fiber* fiber::nextFiber() {
    GUARD(fiberMutex)
    if(lastFiberIndex >= fibers.size()) {
        lastFiberIndex = 0;
    }

    for(; lastFiberIndex < fibers.size(); lastFiberIndex++) {
        fiber *fib = fibers.at(lastFiberIndex);
        if(fib->state == FIB_SUSPENDED && fib->wakeable) {
            lastFiberIndex++;
            return fib;
        }
    }

    return NULL;
}

void fiber::disposeFiber(uInt id) {
    GUARD(fiberMutex)
    fiber *fib = getFiber(id);

    if(fib) {
        fib->free();
        fibers.remove(fib);
        std::free(fib);
    }
}

void fiber::free() {
    if(dataStack != NULL) {
        gc.freeMemory(sizeof(StackElement) * stackLimit);
        StackElement *p = dataStack;
        for(size_t i = 0; i < stackLimit; i++)
        {
            if(p->object.object) {
                DEC_REF(p->object.object);
                p->object.object=NULL;
            }
            p++;
        }

        std::free(this->dataStack); dataStack = NULL;
    }

    if(registers != NULL) {
        gc.freeMemory(sizeof(double) * REGISTER_SIZE);
        std::free(registers); registers = NULL;
    }

    if(callStack != NULL) {
        gc.freeMemory(sizeof(Frame) * internalStackSize);
        std::free(callStack); callStack = NULL;
    }

    fp = NULL;
    sp = NULL;
    name.free();
    id = -1;
}

void fiber::setState(Thread *thread, fiber_state newState, Int delay) {
    GUARD(mutex);

    switch(newState) {
        case FIB_RUNNING:
            thread->lastRanMills = NANO_TOMILL(Clock::realTimeInNSecs());
            state = newState;
            delayTime = -1;
            break;
        case FIB_SUSPENDED:
            delayTime = delay;
            state = newState;
            break;
        case FIB_KILLED:
            state = newState;
            delayTime = -1;
            break;
    }
}

void fiber::setWakeable(bool enable) {
    GUARD(mutex);
    wakeable = enable;
}
