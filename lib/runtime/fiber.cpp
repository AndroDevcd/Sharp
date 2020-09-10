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

fiber* fiber::makeFiber(native_string &name, Method* main) {
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
        fib->delayTime = -1;
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
        return NULL;
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

    uInt loggedTime = NANO_TOMILL(Clock::realTimeInNSecs());
    for(; lastFiberIndex < fibers.size(); lastFiberIndex++) {
        fiber *fib = fibers.at(lastFiberIndex);
        if(fib->state == FIB_SUSPENDED && fib->wakeable) {
            if(fib->delayTime >= 0 && loggedTime >= fib->delayTime) {
                lastFiberIndex++;
                return fib;
            } else if(fib->delayTime == -1) {
                lastFiberIndex++;
                return fib;
            }
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
    GUARD(fiberMutex)
    fiber *fib = getFiber(id);
    int result = 0;

    if(fib) {
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
    GUARD(fiberMutex)
    fiber *fib = getFiber(id);
    int result = 0;

    if(fib) {
        if(fib->state == FIB_SUSPENDED) {
            fib->setWakeable(true);
        } else {
            result = 1;
        }
    }

    return result;
}

int fiber::kill(uInt id) {
    GUARD(fiberMutex)
    fiber *fib = getFiber(id);
    int result = 0;

    if(fib) {
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
    GUARD(fiberMutex)

    bind(NULL);
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
    GUARD(fiberMutex)
    return (Int)state;
}

void fiber::setState(Thread *thread, fiber_state newState, Int delay) {
    GUARD(fiberMutex)

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
            bind(NULL);
            state = newState;
            delayTime = -1;
            break;
    }
}

void fiber::setWakeable(bool enable) {
    GUARD(fiberMutex)
    wakeable = enable;
}

Thread *fiber::getAttachedThread() {
    GUARD(fiberMutex)
    return attachedThread;
}

Thread *fiber::getBoundThread() {
    GUARD(fiberMutex)
    return boundThread;
}

void fiber::setAttachedThread(Thread *thread) {
    GUARD(fiberMutex)
    attachedThread = thread;
}

void fiber::delay(uInt time) {
    GUARD(fiberMutex)
    attachedThread->enableContextSwitch(NULL, true);
    attachedThread->contextSwitching = true;
    setState(thread_self, FIB_SUSPENDED, NANO_TOMILL(Clock::realTimeInNSecs()) + time);
}

int fiber::bind(Thread *thread) {
    GUARD(fiberMutex)

    if(thread != NULL) {
        std::lock_guard<recursive_mutex> guard2(thread->mutex);
        if(thread->state != THREAD_KILLED || !hasSignal(thread->signal, tsig_kill)) {
            boundThread = thread;
            thread->boundFibers++;
            return 0;
        }
    } else {
        if(boundThread)
            boundThread->boundFibers--;
        boundThread = NULL;
        return 0;
    }


    return 1;
}

Int fiber::boundFiberCount(Thread *thread) {
    if(thread != NULL) {
        fiberMutex.lock();
        uInt fibs = thread->boundFibers;
        fiberMutex.unlock();
        return fibs;
    }
    return 0;
}

void fiber::killBoundFibers(Thread *thread) {
    GUARD(fiberMutex)

    for(Int i = 0; i < fibers.size(); i++) {
        fiber *fib = fibers.at(i);
        if(fib->getBoundThread() == thread && fib->state != FIB_KILLED && fib != thread->this_fiber) {
            kill(fib->id);
        }
    }
}

