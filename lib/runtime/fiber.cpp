//
// Created by BNunnally on 9/5/2020.
//
#include "fiber.h"
#include "Thread.h"
#include "VirtualMachine.h"
#include "scheduler.h"

uInt fibId=0 ;
Int dataSize=0, capacity = 0;
const int MAX_PASSES = 1;
const Int RESIZE_MIN = 2500l;
const Int RESIZE_MAX = 25000l;
recursive_mutex fmut;
atomic<fiber**> fibers = { NULL };
atomic<Int> unBoundFibers = { 0 };
atomic<uInt> staleFibers = { 0 };
atomic<uInt> openSpots = { 0 };

#define fiberAt(pos) fibers.load(memory_order_acq_rel)[pos]

void increase_fibers() {
    if((dataSize + 1) >= capacity) {
        GUARD(fmut)
        Int resizeAmnt = capacity == 0 ? RESIZE_MIN : capacity + RESIZE_MIN;
        if(resizeAmnt > RESIZE_MAX) resizeAmnt = RESIZE_MAX;

        fiber** tmpfibers = (fiber**)__calloc((capacity + resizeAmnt), sizeof(fiber**));
        fiber ** old = fibers.load(memory_order_acq_rel);

        fibers.store(tmpfibers);
        for(Int i = 0; i < dataSize; i++)
            tmpfibers[i] = old[i];

        capacity += resizeAmnt;
        free(old);
        gc.addMemory(sizeof(fiber **) * resizeAmnt);
    }
}

void decrease_fibers() {

    Int resizeAmount = (dataSize + (0.25 * capacity));
    if(resizeAmount > RESIZE_MIN && resizeAmount < capacity) {
        GUARD(fmut)
        fiber** tmpfibers = (fiber**)__calloc(resizeAmount, sizeof(fiber**));
        fiber **old = fibers.load(memory_order_acq_rel);

        fibers.store(tmpfibers);
        for(Int i = 0; i < dataSize; i++) {
            tmpfibers[i]=old[i];
        }

        capacity = resizeAmount;
        gc.freeMemory(sizeof(fiber**) * RESIZE_MIN);
        free(old);
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

    if(openSpots > 0) {
        Int size = dataSize;
        for (Int i = 0; i < size; i++) {
            if (fiberAt(i) == NULL) {
                if (fiberAt(i) == NULL) {
                    GUARD(fmut)
                    openSpots--;
                    fib->id = fibId++;
                    fiberAt(i) = fib;
                    fib->itemIndex=i;
                    return;
                }
            }
        }
    }

    if((dataSize+1) >= capacity)
        increase_fibers();

    GUARD(fmut)
    fib->id = fibId++;
    fib->itemIndex = dataSize;
    fiberAt(dataSize++) = fib;
}

fiber* fiber::makeFiber(native_string &name, Method* main) {
    // todo: check if space available before allocation
    // todo: check to see if fib is null first
   fiber *fib = (fiber*)malloc(sizeof(fiber));

   if(fib == nullptr) {
       cout << "null\n" << std::flush << endl;
   }

    try {
        fib->name.init();
        fib->name = name;
        fib->main = main;
        fib->pc = 0;
        fib->state = FIB_CREATED;
        fib->exitVal = 0;
        fib->delayTime = -1;
        fib->wakeable = true;
        fib->finished = false;
        fib->locking = false;
        fib->marked = false;
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
        fib->passed = 0;
        fib->stackLimit = internalStackSize;
        fib->registers = (double *) __calloc(REGISTER_SIZE, sizeof(double));
        new (&fib->mut) std::recursive_mutex();

        if(internalStackSize < INITIAL_STACK_SIZE) {
            fib->stackSize = internalStackSize;
        }
        else if((vm.manifest.threadLocals + 50) < INITIAL_STACK_SIZE) {
            fib->stackSize = INITIAL_STACK_SIZE;
        }
        else {
            fib->stackSize = vm.manifest.threadLocals + INITIAL_STACK_SIZE;
        }

        fib->dataStack = (StackElement *) __malloc(fib->stackSize* sizeof(StackElement));
        StackElement *ptr = fib->dataStack;
        for(Int i = 0; i < fib->stackSize; i++) {
            ptr->object.object = NULL;
            ptr->var=0;
            ptr++;
        }

        if(internalStackSize - vm.manifest.threadLocals < INITIAL_FRAME_SIZE) {
            fib->frameSize = internalStackSize - vm.manifest.threadLocals;
            fib->callStack = (Frame *) __malloc(fib->frameSize * sizeof(Frame));
        }
        else {
            fib->frameSize = INITIAL_FRAME_SIZE;
            fib->callStack = (Frame *) __malloc(fib->frameSize * sizeof(Frame));
        }

        fib->frameLimit = internalStackSize - vm.manifest.threadLocals;
        fib->fp = &fib->dataStack[vm.manifest.threadLocals];
        fib->sp = (&fib->dataStack[vm.manifest.threadLocals]) - 1;

        for(Int i = 0; i < vm.tlsInts.size(); i++) {
            fib->dataStack[vm.tlsInts._Data[i].key].object =
                    gc.newObject(1, vm.tlsInts._Data[i].value);
        }

        unBoundFibers++;
        gc.addMemory(sizeof(Frame) * fib->frameSize);
        gc.addMemory(sizeof(StackElement) * fib->stackSize);
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

bool fiber::isFiberRunnble(fiber *fib, Thread *thread) {

    if(fib->state == FIB_SUSPENDED && fib->wakeable) {
        return (fib->boundThread == thread || fib->boundThread == NULL);
    }

    return false;
}

fiber* fiber::nextFiber(Int startingIndex, Thread *thread) {
    fiber *fib = NULL;
    uInt loggedTime = NANO_TOMILL(Clock::realTimeInNSecs());
    Int size;

    if(unBoundFibers > 0 || thread->boundFibers > 0) {
        if (startingIndex >= 0 && (startingIndex + 1) < dataSize) {

            for (Int i = startingIndex + 1; i < dataSize; i++) {
                if (thread->signal) return NULL;

                if ((fib = fiberAt(i)) != NULL && !fib->finished && isFiberRunnble(fib, thread)) {
                    if (fib->locking) {
                        if (fib->passed >= MAX_PASSES)
                            fib->passed = 0;
                        else {
                            fib->passed++;
                            continue;
                        }
                    }

                    if (fib->delayTime > 0 && loggedTime < fib->delayTime) continue;
                    return fib;
                }
            }

            size = startingIndex + 1;
        } else size = dataSize;

        for (Int i = 0; i < size; i++) {
            if (thread->signal) return NULL;

            if ((fib = fiberAt(i)) != NULL && !fib->finished && isFiberRunnble(fib, thread)) {
                if (fib->delayTime > 0 && loggedTime < fib->delayTime) continue;
                return fib;
            }
        }
    } else if(!thread->last_fiber->finished && isFiberRunnble(thread->last_fiber, thread)) {
        if (thread->last_fiber->delayTime > 0 && loggedTime < thread->last_fiber->delayTime) return NULL;
        return thread->last_fiber;
    }

    return NULL;
}

void fiber::disposeFibers() {
    if(staleFibers.load() > 0) {
        for (Int i = 0; i < dataSize; i++) {
            fiber *fib = fiberAt(i);

            if (vm.state >= VM_SHUTTING_DOWN) {
                return;
            }

            if (fib && fib->finished && fib->state == FIB_KILLED && fib->attachedThread == NULL) {
                if(fib->marked) {
                    fiberAt(i) = NULL;
                    GUARD(fmut)
                    staleFibers--;
                    openSpots++;

                    fib->free();
                    std::free(fib);
                } else fib->marked = true;
            }
        }

        decrease_fibers();
    }

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
        GUARD(fib->mut)
        if(fib->state == FIB_SUSPENDED) {
            fib->setState(NULL, FIB_KILLED);
        } else if(fib->state == FIB_RUNNING) {
            if(fib->attachedThread) {
                fib->setState(NULL, FIB_KILLED);
                fib->attachedThread->enableContextSwitch(true);
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
    gc.reconcileLocks(this);

    bind(NULL);
    if(dataStack != NULL) {
        gc.freeMemory(sizeof(StackElement) * stackSize);
        StackElement *p = dataStack;
        for(size_t i = 0; i < stackSize; i++)
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
        case FIB_SUSPENDED: {

            if(delay > 0) {
                delayTime = NANO_TOMILL(Clock::realTimeInNSecs()) + delay;
            } else delayTime = -1;
            state = newState;
            break;
        }
        case FIB_KILLED:
            bind(NULL);
            state = newState;
            finished = true;
            delayTime = -1;
            staleFibers++;
            break;
    }
    
}

void fiber::setWakeable(bool enable) {
    GUARD(mut)
    wakeable = enable;
}

Thread *fiber::getAttachedThread() {
    GUARD(mut)
    return attachedThread;
}

Thread *fiber::getBoundThread() {
    GUARD(mut)
    return boundThread;
}

void fiber::setAttachedThread(Thread *thread) {
    GUARD(mut)
    attachedThread = thread;
}

void fiber::delay(Int time, bool incPc) {
    if(time < 0)
        time = -1;


    if(thread_self->try_context_switch(incPc)) {
        thread_self->enableContextSwitch(true);
        if (state != FIB_KILLED)
            setState(thread_self, FIB_SUSPENDED, time);
    } else  {
        __usleep(time * 1000); // will become a sleep() call if called from c++ env
    }
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
            unBoundFibers--;
            boundThread = thread;
            thread->boundFibers++;
            
            return 0;
        }
    } else {
        if(boundThread) {
            std::lock_guard<recursive_mutex> guard2(boundThread->mutex);
            unBoundFibers++;
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
        return thread->boundFibers;
    }
    return 0;
}

void fiber::killBoundFibers(Thread *thread) {
    fiber *fib = NULL;

    for(Int i = 0; i < dataSize; i++) {
        if((fib = fiberAt(i)) != NULL && fib->getBoundThread() == thread && fib->state != FIB_KILLED && fib != thread->this_fiber) {
            kill(fib->id);
        }
    }
}

/*
 * foo(var, var, string) { [5 locals] } [3]
 *
 * 5
 * 10
 * "hello"
 */

void fiber::growFrame() {
    GUARD(mut)

    if(frameSize + FRAME_GROW_SIZE < frameLimit) {
        callStack = (Frame *) __realloc(callStack, sizeof(Frame) * (frameSize + FRAME_GROW_SIZE),
                                        sizeof(Frame) * frameSize);
        frameSize += FRAME_GROW_SIZE;
        gc.addMemory(sizeof(Frame) * FRAME_GROW_SIZE);
    }
    else if(frameSize != frameLimit) {
        callStack = (Frame *) __realloc(callStack, sizeof(Frame) * (frameLimit), sizeof(Frame) * frameSize);
        gc.addMemory(sizeof(Frame) * (frameLimit - frameSize));
        frameSize = frameLimit;
    }
    
}

void fiber::growStack(Int requiredSize) {
    GUARD(mut)
    if(requiredSize < STACK_GROW_SIZE)
      requiredSize = STACK_GROW_SIZE;
    else requiredSize += STACK_GROW_SIZE;

    if(stackSize + requiredSize < stackLimit) {
        Int spIdx = sp-dataStack, fpIdx = fp-dataStack, ptrIdx = -1;
        if(ptr) {
            for(Int i = 0; i < stackSize; i++) {
                if(ptr == &dataStack[i].object)
                {
                    ptrIdx = i;
                    break;
                }
            }
        }

        dataStack = (StackElement *) __realloc(dataStack, sizeof(StackElement) * (stackSize + requiredSize),
                                        sizeof(StackElement) * stackSize);

        StackElement *stackItem = dataStack+stackSize;
        for(Int i = stackSize; i < stackSize+requiredSize; i++) {
            stackItem->object.object = NULL;
            stackItem->var=0;
            stackItem++;
        }

        if(ptrIdx >= 0) ptr = &dataStack[ptrIdx].object;
        sp = dataStack+spIdx;
        fp = dataStack+fpIdx;
        stackSize += requiredSize;
        gc.addMemory(sizeof(StackElement) * requiredSize);
    }
    else if(stackSize != stackLimit) {
        Int spIdx = sp-dataStack, fpIdx = fp-dataStack, ptrIdx = -1;
        if(ptr) {
            for(Int i = 0; i < stackSize; i++) {
                if(ptr == &dataStack[i].object)
                {
                    ptrIdx = i;
                    break;
                }
            }
        }

        dataStack = (StackElement *) __realloc(dataStack, sizeof(StackElement) * (stackLimit), sizeof(StackElement) * stackSize);
        gc.addMemory(sizeof(Frame) * (stackLimit - stackSize));

        StackElement *stackItem = dataStack+stackSize;
        for(Int i = stackSize; i < stackLimit; i++) {
            stackItem->object.object = NULL;
            stackItem->var=0;
            stackItem++;
        }

        if(ptrIdx >= 0) ptr = &dataStack[ptrIdx].object;
        sp = dataStack+spIdx;
        fp = dataStack+fpIdx;
        stackSize = stackLimit;
    }

}

