//
// Created by BNunnally on 9/5/2020.
//
#include "fiber.h"
#include "Thread.h"
#include "VirtualMachine.h"

uInt fibId=0 ;
Int dataSize=0, capacity = 0;
const Int RESIZE_MIN = 2500l;
recursive_mutex fmut;
fiber** fibers;

#define fiberAt(pos) fibers[pos]

void increase_fibers() {
    if((dataSize + 1) >= capacity) {
        Thread::suspendAllThreads(true);
        GUARD(fmut)
        Int resizeAmnt = capacity == 0 ? RESIZE_MIN : (capacity >> 4) + RESIZE_MIN;
        fibers = (fiber**)__realloc(fibers, (capacity + resizeAmnt) * sizeof(fiber **), (capacity) * sizeof(fiber **));
        for(Int i = capacity; i < (capacity+resizeAmnt); i++)
            fibers[i]=NULL;

        capacity += resizeAmnt;
        gc.addMemory(sizeof(fiber **) * resizeAmnt);
        Thread::resumeAllThreads(true);
    }
}

void decrease_fibers() {
    GUARD(fmut)

    if(dataSize > RESIZE_MIN && (capacity - dataSize) >= RESIZE_MIN) {
        Thread::suspendAllThreads(true);
        fiber** tmpfibers = (fiber**)__calloc((capacity - RESIZE_MIN), sizeof(fiber**)), **old;
        for(Int i = 0; i < dataSize; i++) {
            tmpfibers[i]=fibers[i];
        }

        dataSize -= RESIZE_MIN;
        old = fibers;
        fibers=tmpfibers;
        std::free(old);
        gc.freeMemory(sizeof(fiber**) * RESIZE_MIN);
        Thread::resumeAllThreads(true);
    }
}

fiber* locateFiber(Int id) {
    Int size = dataSize;
    for(Int i = 0; i < size; i++) {

        if(fiberAt(i) && fiberAt(i)->id == id) {
            return fiberAt(i);
        }
    }

    return nullptr;
}

void addFiber(fiber* fib) {
    fib->id = fibId++;

    Int size = dataSize;
    for(Int i = 0; i < size; i++) {
        if(fibers[i] == NULL) {
            GUARD(fmut)
            if(fibers[i] == NULL) {
                fibers[i] = fib;
                return;
            }
        }
    }

    if(dataSize >= capacity)
        increase_fibers();

    fibers[dataSize++] = fib;
}

fiber* fiber::makeFiber(native_string &name, Method* main) {
    // todo: check if space available before allocation
   fiber *fib = (fiber*)malloc(sizeof(fiber));

    try {
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
        new (&fib->mut) std::recursive_mutex();

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
    return locateFiber(id);
}

inline bool isFiberRunnble(fiber *fib, Int loggedTime, Thread *thread) {
    if(fib->state == FIB_SUSPENDED && fib->wakeable && fib->attachedThread == NULL) {
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

    fiber *fib = NULL;
    uInt loggedTime = NANO_TOMICRO(Clock::realTimeInNSecs());
    if(startingFiber != NULL) {
        for (Int i = 0; i < dataSize; i++) {
            if ((fib = fiberAt(i)) != NULL && fib == startingFiber) {
                if ((i + 1) < dataSize) {
                    for (Int j = i + 1; j < dataSize; j++) {
                        if ((fib = fiberAt(j)) != NULL && !fib->finished && !fib->locking && isFiberRunnble(fib, loggedTime, thread)) {
                            return fib;
                        }
                    }

                    break;
                } else break;
            }
        }
    }

    for(Int i = 0; i < dataSize; i++) {
       if((fib = fiberAt(i)) != NULL && !fib->finished && fib != startingFiber && isFiberRunnble(fib, loggedTime, thread))
           return fib;
    }

    return startingFiber;
}

void fiber::dispose(fiber *f) {
    GUARD(fmut)
    {

        for(Int i = 0; i < dataSize; i++) {

            if(fiberAt(i)== f) {
                fiberAt(i) = NULL;
            }
        }
    }

    f->free();
    std::free(f);
    decrease_fibers();
}


int fiber::suspend(uInt id) {
    fiber *fib = getFiber(id);
    int result = 0;

    if(fib) {
        GUARD(fib->mut)
        if(fib->state == FIB_SUSPENDED) {
            fib->setWakeable(false);
        } else if(fib->state == FIB_RUNNING) {
            if(fib->attachedThread) {
                fib->setWakeable(false);
                fib->setState(fib->attachedThread, FIB_SUSPENDED);
                fib->attachedThread->enableContextSwitch(true);
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
        GUARD(fib->mut)
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
        GUARD(fmut)
        if(fib->state == FIB_SUSPENDED) {
            dispose(fib);
        } else if(fib->state == FIB_RUNNING) {
            if(fib->attachedThread) {
                fib->setState(NULL, FIB_KILLED);
                fib->attachedThread->enableContextSwitch(true);
            } else {
                result = 2;
            }
        } else {
            dispose(fib);
        }
        
    }

    return result;
}

void fiber::free() {
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
    GUARD(mut)
    auto result = (Int)state;
    
    return result;
}

void fiber::setState(Thread *thread, fiber_state newState, Int delay) {
    GUARD(mut)

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
            finished = true;
            delayTime = -1;
            break;
    }
    
}

void fiber::setWakeable(bool enable) {
    GUARD(mut)
    wakeable = enable;
}

Thread *fiber::getAttachedThread() {
    GUARD(mut)
    auto result = attachedThread;
    
    return result;
}

Thread *fiber::getBoundThread() {
    GUARD(mut)
    auto result = boundThread;
    
    return result;
}

void fiber::setAttachedThread(Thread *thread) {
    GUARD(mut)
    attachedThread = thread;
}

void fiber::delay(Int time) {
    if(time < 0)
        time = -1;

    attachedThread->enableContextSwitch(true);
    attachedThread->contextSwitching = true;
    setState(thread_self, FIB_SUSPENDED, NANO_TOMICRO(Clock::realTimeInNSecs()) + time);
}

bool fiber::safeStart(Thread *thread) {
    GUARD(mut)
    if(state == FIB_SUSPENDED && attachedThread == NULL && (boundThread == NULL || boundThread == thread)) {
        attachedThread = (thread);
        setState(thread, FIB_RUNNING);
        return true;
    } else {
        
        return false;
    }
}

int fiber::bind(Thread *thread) {
    GUARD(mut)

    if(thread != NULL) {
        std::lock_guard<recursive_mutex> guard2(thread->mutex);
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
        GUARD(thread->mutex)
        uInt fibs = thread->boundFibers;
        return fibs;
    }
    return 0;
}

void fiber::killBoundFibers(Thread *thread) {
    fiber *fib = NULL;

    for(Int i = 0; i <= fibId; i++) {
        if((fib = getFiber(i)) != NULL && fib->getBoundThread() == thread && fib->state != FIB_KILLED && fib != thread->this_fiber) {
            kill(fib->id);
        }
    }
}

void fiber::growFrame() {
    GUARD(mut)

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

