//
// Created by BNunnally on 9/5/2020.
//

#include "../../stdimports.h"
#include "Thread.h"
#include "linked_list.h"
#include "scheduler.h"
#include "VirtualMachine.h"

void run_scheduler() {
    do {
       fiber *fib = NULL;

        __os_yield();
       while((fib = fiber::nextFiber()) != NULL) {

           if(fib->delayTime >= 0) {
               if(NANO_TOMILL(Clock::realTimeInNSecs()) >= fib->delayTime) {
                   try_context_switch(fib);
               }
           } else {
               try_context_switch(fib);
           }
       }

       if(vm.state == VM_SHUTTING_DOWN) {
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

       __os_sleep(LPTSI);
    } while(true);
}

bool try_context_switch(Thread *thread, fiber *fib) {
    if(fib->state != FIB_RUNNING) {
        return false; // race condition protect
    }

    thread->enableContextSwitch(fib, true);

    const long sMaxRetries = 10000000;
    long retryCount = 0;
    uInt startTime = NANO_TOMILL(Clock::realTimeInNSecs());
    while(thread->next_fiber != NULL) {
        if (retryCount++ == sMaxRetries)
        {
            retryCount = 0;
            __os_yield();
#ifdef WIN32_
            Sleep(1);
#endif
#ifdef POSIX_
            usleep(1*POSIX_USEC_INTERVAL);
#endif
        } else if((NANO_TOMILL(Clock::realTimeInNSecs()) - startTime) > CSTL) {
            thread->enableContextSwitch(NULL, false);
            return false;
        } else if(thread->state == THREAD_KILLED)
            return false;
    }

    return true;
}

bool is_thread_ready(Thread *thread) {
    uInt currentTime = NANO_TOMILL(Clock::realTimeInNSecs());

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

    for(Int i = 0; i < Thread::threads.size(); i++) {
        thread = Thread::threads.at(i);
        if(vm.state >= VM_SHUTTING_DOWN) {
            return false;
        }

        if(thread->state == THREAD_RUNNING && is_thread_ready(thread)) {
            if(try_context_switch(thread, fib)) {
                return true;
            }
        }
    }

    return false;
}

extern void printRegs();

void delay_fib(fiber* fib, uInt time) {
    cout << "delay " << time << endl << std::flush;

    thread_self->enableContextSwitch(NULL, true);
    thread_self->contextSwitching = true;
    fib->setState(thread_self, FIB_SUSPENDED, NANO_TOMILL(Clock::realTimeInNSecs()) + time);
}