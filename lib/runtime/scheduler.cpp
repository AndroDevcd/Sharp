//
// Created by BNunnally on 9/5/2020.
//

#include "../../stdimports.h"
#include "Thread.h"
#include "linked_list.h"
#include "scheduler.h"
#include "VirtualMachine.h"

uInt loggedTime = 0;
void run_scheduler() {
    do {
       fiber *fib = NULL;

       __os_yield();
       while((fib = fiber::nextFiber()) != NULL) {
           loggedTime = NANO_TOMILL(Clock::realTimeInNSecs());
           if(vm.state >= VM_SHUTTING_DOWN) {
               break;
           }

           try_context_switch(fib);
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
    if(thread->state != THREAD_RUNNING || hasSignal(thread->signal, tsig_kill))
        return false;

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

bool try_context_switch(fiber *fib) {
    GUARD(Thread::threadsListMutex);
    Thread *thread;

    if((thread = fib->getBoundThread()))  {
        return is_thread_ready(thread) && try_context_switch(thread, fib);
    }

    for(Int i = 0; i < Thread::threads.size(); i++) {
        thread = Thread::threads.at(i);
        if(vm.state >= VM_SHUTTING_DOWN) {
            return false;
        }

        if(vm.state != VM_SHUTTING_DOWN && thread->state == THREAD_KILLED) {
            Thread::destroy(thread); i--;
            continue;
        }

        if(is_thread_ready(thread)) {
            if(try_context_switch(thread, fib)) {
                return true;
            }
        }
    }

    return false;
}
