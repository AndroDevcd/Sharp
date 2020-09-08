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

fiber* fiber::makeFiber(string name, Method* main) {
    GUARD(fiberMutex)
    // todo: check if space available before allocation
   fiber *fib = (fiber*)malloc(sizeof(fiber));
   fibers.add(fib);

    try {
        fib->id = fibId++;
        fib->name.init();
        fib->name = name;
        fib->main = main;
        fib->cache = NULL;
        fib->pc = NULL;
        fib->pc = NULL;
        fib->state = FIB_CREATED;
        fib->exitVal = 0;
        fib->wakeable = true;
        fib->finished = false;
        fib->attachedThread = NULL;
        fib->boundThread = NULL;
        fib->exceptionObject.object = NULL;
        fib->fiberObject.object = NULL;
        fib->dataStack = NULL;
        fib->registers = NULL;
        fib->callStack = NULL;
        fib->calls = -1;
        new(&fib->mutex) std::recursive_mutex();
        fib->stackLimit = internalStackSize;
        fib->registers = (double *) calloc(REGISTER_SIZE, sizeof(double));
        fib->dataStack = (StackElement *) calloc(internalStackSize, sizeof(StackElement));
        fib->callStack = (Frame *) __calloc(internalStackSize - vm.manifest.threadLocals, sizeof(Frame));
        fib->fp = &fib->dataStack[vm.manifest.threadLocals];
        fib->sp = (&fib->dataStack[vm.manifest.threadLocals]) - 1;

        gc.addMemory(sizeof(Frame) * (internalStackSize - vm.manifest.threadLocals));
        gc.addMemory(sizeof(StackElement) * internalStackSize);
        gc.addMemory(sizeof(double) * REGISTER_SIZE);
    } catch(Exception &e) {
        fib->state = FIB_KILLED;
    }
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
    if(lastFiberIndex >= fibers.size() || lastFiberIndex < 0) {
        lastFiberIndex = 0;
    }

    for(; lastFiberIndex < fibers.size(); lastFiberIndex++) {
        fiber *fib = fibers.at(lastFiberIndex);
        if(fib->state == FIB_SUSPENDED && fib->wakeable) {
            lastFiberIndex++;
            return fib;
        } else if(fib->state == FIB_KILLED) {
            disposeFiber(fib);
            lastFiberIndex--;
        }
    }

    return NULL;
}

void fiber::disposeFiber(fiber *fib) {
    fib->free();
    fibers.remove(fib);
    std::free(fib);
}

int fiber::suspend(uInt id) {
    fiber *fib;
    int result = 0;

    {
        GUARD(fiberMutex)
        fib = getFiber(id);
    }

    if(fib) {
        GUARD(fib->mutex)
        if(fib->state == FIB_SUSPENDED) {
            fib->setWakeable(false);
        } else if(fib->state == FIB_RUNNING) {
            if(fib->attachedThread) {
                fib->setWakeable(false);
                fib->setState(fib->attachedThread, FIB_SUSPENDED);
                fib->attachedThread->enableContextSwitch(NULL, true);
            } else {
                result = 2;
            }
        } else {
            result = 1;
        }
    }

    return result;
}

int fiber::unsuspend(uInt id) {
    fiber *fib;
    int result = 0;

    {
        GUARD(fiberMutex)
        fib = getFiber(id);
    }

    if(fib) {
        GUARD(fib->mutex)
        if(fib->state == FIB_SUSPENDED) {
            fib->setWakeable(true);
        } else {
            result = 1;
        }
    }

    return result;
}

int fiber::kill(uInt id) {
    fiber *fib;
    int result = 0;

    {
        GUARD(fiberMutex)
        fib = getFiber(id);
    }

    if(fib) {
        GUARD(fib->mutex)
        if(fib->state == FIB_SUSPENDED) {
            fib->setState(NULL, FIB_KILLED);
        } else if(fib->state == FIB_RUNNING) {
            if(fib->attachedThread) {
                fib->setState(NULL, FIB_KILLED);
                fib->attachedThread->enableContextSwitch(NULL, true);
            } else {
                result = 2;
            }
        } else {
            fib->setState(NULL, FIB_KILLED);
        }
    }

    return result;
}

void fiber::free() {
    GUARD(mutex);
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
        gc.freeMemory(sizeof(Frame) * (internalStackSize - vm.manifest.threadLocals));
        std::free(callStack); callStack = NULL;
    }

    fp = NULL;
    sp = NULL;
    name.free();
    id = -1;
}

int fiber::getState() {
    GUARD(mutex);
    return (Int)state;
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

Thread *fiber::getAttachedThread() {
    GUARD(mutex);
    return attachedThread;
}

Thread *fiber::getBoundThread() {
    GUARD(mutex);
    return boundThread;
}

void fiber::setAttachedThread(Thread *thread) {
    GUARD(mutex);
    attachedThread = thread;
}

void fiber::delay(uInt time) {
    attachedThread->enableContextSwitch(NULL, true);
    attachedThread->contextSwitching = true;
    setState(thread_self, FIB_SUSPENDED, NANO_TOMILL(Clock::realTimeInNSecs()) + time);
}

int fiber::bind(Thread *thread) {
    if(thread != NULL) {
        GUARD(mutex);
        std::lock_guard<recursive_mutex> guard2(thread->mutex);
        if(thread->state != THREAD_KILLED) {
            boundThread = thread;
            return 0;
        }
    }

    return 1;
}

Int fiber::boundFiberCount(Thread *thread) {
    GUARD(fiberMutex)
    Int boundFibers = 0;

    for(Int i = 0; i < fibers.size(); i++) {
        fiber *fib = fibers.at(i);
        if(fib->getBoundThread() == thread && fib->state != FIB_KILLED) {
            boundFibers++;
        }
    }

    return boundFibers;
}

void fiber::killBoundFibers(Thread *thread) {
    GUARD(fiberMutex)

    for(Int i = 0; i < fibers.size(); i++) {
        fiber *fib = fibers.at(i);
        if(fib->getBoundThread() == thread && fib->state != FIB_KILLED) {
            kill(fib->id);
        }
    }
}

