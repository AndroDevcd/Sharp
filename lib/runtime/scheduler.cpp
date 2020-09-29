//
// Created by BNunnally on 9/5/2020.
//

#include "../../stdimports.h"
#include "Thread.h"
#include "linked_list.h"
#include "scheduler.h"
#include "VirtualMachine.h"

atomic<bool> threadReleaseBlock = { false };
uInt loggedTime = 0;
void run_scheduler() {
    do {
       fiber *fib = NULL;
       Thread *thread;
       Int size;

       __os_yield();
        {
            GUARD(Thread::threadsListMutex);
            size = Thread::threads.size();
        }

        for (Int i = 0; i < size; i++) {
            thread = Thread::threads.at(i);
            loggedTime = NANO_TOMILL(Clock::realTimeInNSecs());

            if (vm.state >= VM_SHUTTING_DOWN) {
                break;
            }

            if (vm.state != VM_SHUTTING_DOWN && thread->state == THREAD_KILLED) {
                GUARD(Thread::threadsListMutex);
                if(!threadReleaseBlock) {
                    fiber::killBoundFibers(thread);
                    Thread::destroy(thread);
                    i--;
                }
                continue;
            }

            fib = fiber::nextFiber(thread->last_fiber, thread);
            if (fib != NULL && is_thread_ready(thread)) {
                try_context_switch(thread, fib);
            }
        }

       if(vm.state >= VM_SHUTTING_DOWN) {
           while(vm.state != VM_TERMINATED) {
#ifdef WIN32_
               Sleep(1);
#endif
#ifdef POSIX_
               usleep(1*POSIX_USEC_INTERVAL);
#endif
           }
           return;
       }

#ifdef WIN32_
        Sleep(5);
#endif
#ifdef POSIX_
        usleep(5*POSIX_USEC_INTERVAL);
#endif
    } while(true);
}

bool try_context_switch(Thread *thread, fiber *fib) {
    if(fib->state != FIB_SUSPENDED) {
        return false; // race condition protect
    }

    thread->enableContextSwitch(fib, true);

    const long sMaxRetries = 5000000;
    long retryCount = 0;
    long spinCount = 0;
    while(thread->next_fiber != NULL) {
        if (retryCount++ == sMaxRetries)
        {
            spinCount++;
            retryCount = 0;
            __os_yield();
#ifdef WIN32_
            Sleep(1);
#endif
#ifdef POSIX_
            usleep(1*POSIX_USEC_INTERVAL);
#endif
        } else if(spinCount >= CSTL) {
            thread->enableContextSwitch(NULL, false);
            return false;
        } else if(thread->state == THREAD_KILLED || hasSignal(thread->signal, tsig_kill))
            return false;
    }

    return true;
}

bool is_thread_ready(Thread *thread) {
    uInt currentTime = loggedTime;
    if(thread->state != THREAD_RUNNING || hasSignal(thread->signal, tsig_kill) || hasSignal(thread->signal, tsig_suspend))
        return false;
    if(thread->this_fiber == NULL) return true;

    switch(thread->priority) {
        case THREAD_PRIORITY_LOW:
            return (currentTime - thread->lastRanMills) > LPTSI;
        case THREAD_PRIORITY_NORM:
            return (currentTime - thread->lastRanMills) > NPTSI;
        case THREAD_PRIORITY_HIGH:
            return (currentTime - thread->lastRanMills) > HPTSI;
    }

    return false;
}
