//
// Created by BNunnally on 9/5/2020.
//
#include "fiber.h"
#include "Thread.h"
#include "VirtualMachine.h"

uInt fiber::fibId=0;
uInt listSize = 0, fiberCount =0, resizeCount = 500;
recursive_mutex fiberMutex, reallocMutex;
fiber** fibers=NULL;

void increase_fibers() {
    GUARD(reallocMutex)
    fibers = (fiber**)__realloc(fibers, sizeof(fiber**) * (listSize + resizeCount), sizeof(fiber**) * listSize);
    for(Int i = listSize; i < (listSize+resizeCount); i++) {
        fibers[i]=NULL;
    }

    listSize += resizeCount;
    gc.addMemory(sizeof(fiber**) * resizeCount);
}

void decrease_fibers() {
    if(listSize > resizeCount && (listSize - fiberCount) >= resizeCount) {
        GUARD(reallocMutex)
        auto result = (fiber **) realloc(fibers, sizeof(fiber **) * (listSize - resizeCount));

        if(result) {
            listSize -= resizeCount;
            fibers = result;
            gc.freeMemory(sizeof(fiber**) * resizeCount);
        }
    }
}

fiber* locateFiber(Int id) {
    for(Int i = 0; i < fiberCount; i++) {
        if(fibers[i] && fibers[i]->id == id) {
            return fibers[i];
        }
    }

    return nullptr;
}

void addFiber(fiber* fib) {
    for(Int i = 0; i < fiberCount; i++) {
        if(fibers[i] == NULL) {
            fibers[i] = fib;
            return;
        }
    }

    if(fiberCount >= listSize)
        increase_fibers();

    fibers[fiberCount++] = fib;
}

void removeFiber(fiber* fib) {
    for(Int i = 0; i < fiberCount; i++) {
        if(fibers[i] == fib) {
            fib->free();
            std::free(fib);
            fibers[i] = NULL;
            break;
        }
    }

    //decrease_fibers();
}

fiber* fiber::makeFiber(native_string &name, Method* main) {
    GUARD(fiberMutex)
    // todo: check if space available before allocation
   fiber *fib = (fiber*)malloc(sizeof(fiber));

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
        fib->locking = false;
        fib->attachedThread = NULL;
        fib->boundThread = NULL;
        fib->exceptionObject.object = NULL;
        fib->fiberObject.object = NULL;
        fib->dataStack = NULL;
        fib->registers = NULL;
        fib->callStack = NULL;
        fib->calls = -1;
        fib->current = NULL;
        fib->ptr = NULL;
        fib->stackLimit = internalStackSize;
        fib->registers = (double *) __calloc(REGISTER_SIZE, sizeof(double));
        fib->dataStack = (StackElement *) __calloc(internalStackSize, sizeof(StackElement));
        if(internalStackSize - vm.manifest.threadLocals <= INITIAL_FRAME_SIZE) {
            fib->frameSize = internalStackSize - vm.manifest.threadLocals;
            fib->callStack = (Frame *) __calloc(internalStackSize - vm.manifest.threadLocals, sizeof(Frame));
        }
        else {
            fib->frameSize = INITIAL_FRAME_SIZE;
            fib->callStack = (Frame *) __calloc(INITIAL_FRAME_SIZE, sizeof(Frame));
        }

        fib->frameLimit = internalStackSize - vm.manifest.threadLocals;
        fib->fp = &fib->dataStack[vm.manifest.threadLocals];
        fib->sp = (&fib->dataStack[vm.manifest.threadLocals]) - 1;

        for(Int i = 0; i < vm.tlsInts.size(); i++) {
            fib->dataStack[vm.tlsInts.at(i).key].object =
                    gc.newObject(1, vm.tlsInts.at(i).value);
        }

        gc.addMemory(sizeof(Frame) * fib->frameSize);
        gc.addMemory(sizeof(StackElement) * internalStackSize);
        gc.addMemory(sizeof(double) * REGISTER_SIZE);
        addFiber(fib);
    } catch(Exception &e) {
        fib->free();
        std::free(fib);
        throw;
    }
    return fib;
}

fiber* fiber::getFiber(uInt id) {
    GUARD(fiberMutex)
    std::lock_guard<recursive_mutex> guard2(reallocMutex);
    return locateFiber(id);
}

bool isFiberRunnble(fiber *fib, Int loggedTime, Thread *thread) {
    if(fib->state == FIB_SUSPENDED && fib->wakeable) {
        if(fib->boundThread == thread || fib->boundThread == NULL) {
            if (fib->delayTime >= 0 && loggedTime >= fib->delayTime) {
                return true;
            } else if (fib->delayTime <= 0) {
                return true;
            }
        }
    }

    return false;
}

fiber* fiber::nextFiber(fiber *startingFiber, Thread *thread) {
    Int size;

    {
        GUARD(fiberMutex);
        size = fiberCount;
    }

    GUARD(reallocMutex)


    uInt loggedTime = NANO_TOMICRO(Clock::realTimeInNSecs());
    if(startingFiber != NULL) {
        for (Int i = 0; i < size; i++) {
            if (fibers[i] == startingFiber) {
                if ((i + 1) < size) {
                    for (Int j = i + 1; j < size; j++) {
                        if (fibers[j] && !fibers[j]->locking && isFiberRunnble(fibers[j], loggedTime, thread)) {
                            return fibers[j];
                        }
                    }

                    break;
                } else break;
            }
        }
    }

    for(Int i = 0; i < size; i++) {
        fiber* fib = fibers[i];
       if(fib && isFiberRunnble(fib, loggedTime, thread))
           return fib;
       else if(fib && fib->state == FIB_KILLED && fib->boundThread == NULL && fib->attachedThread == NULL) {
           fiber::disposeFiber(fib);
           size--; i--;
       }
    }

    return NULL;
}

void fiber::disposeFiber(fiber *fib) {
    GUARD(fiberMutex)
    removeFiber(fib);
}

int fiber::suspend(uInt id) {
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
    gc.reconcileLocks(this);

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
        gc.freeMemory(sizeof(Frame) * frameSize);
        std::free(callStack); callStack = NULL;
    }

    fiberObject=(SharpObject*)NULL;
    exceptionObject=(SharpObject*)NULL;
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
            thread->lastRanMicros = NANO_TOMICRO(Clock::realTimeInNSecs());
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

void fiber::delay(Int time) {
    GUARD(fiberMutex)
    if(time < 0)
        time = -1;

    attachedThread->enableContextSwitch(thread_self->next_fiber, true);
    attachedThread->contextSwitching = true;
    setState(thread_self, FIB_SUSPENDED, NANO_TOMICRO(Clock::realTimeInNSecs()) + time);
}

extern void printRegs();
int fiber::bind(Thread *thread) {
    GUARD(fiberMutex)

    if(thread != NULL) {
        std::lock_guard<recursive_mutex> guard2(thread->mutex);
        if(thread->boundFibers > 1000) {
            printRegs();
            int i = 0;
        }
        if(thread->state != THREAD_KILLED || !hasSignal(thread->signal, tsig_kill)) {
            boundThread = thread;
            thread->boundFibers++;
            return 0;
        }
    } else {
        if(boundThread) {
            std::lock_guard<recursive_mutex> guard2(boundThread->mutex);
            boundThread->boundFibers--;
        }

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
    Int size;

    {
        GUARD(fiberMutex)
        size = fiberCount;
    }

    for(Int i = 0; i < size; i++) {
        fiber *fib = fibers[i];
        if(fib && fib->getBoundThread() == thread && fib->state != FIB_KILLED && fib != thread->this_fiber) {
            kill(fib->id);
        }
    }
}

void fiber::growFrame() {
    GUARD(fiberMutex)
    if(frameSize + FRAME_GROW_SIZE < frameLimit) {
        callStack = (Frame *) __realloc(callStack, sizeof(Frame) * (frameSize + FRAME_GROW_SIZE),
                                        sizeof(Frame) * frameSize);
        frameSize += FRAME_GROW_SIZE;
        gc.addMemory(sizeof(Frame) * FRAME_GROW_SIZE);
    }
    else {
        callStack = (Frame *) __realloc(callStack, sizeof(Frame) * (frameLimit), sizeof(Frame) * frameSize);
        gc.addMemory(sizeof(Frame) * (frameLimit - frameSize));
        frameSize = frameLimit;
    }
}

