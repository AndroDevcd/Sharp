//
// Created by BNunnally on 9/5/2020.
//

#include "../../stdimports.h"
#include "Thread.h"
#include "linked_list.h"
#include "scheduler.h"
#include "VirtualMachine.h"

void __usleep(unsigned int usec)
{

#ifdef WIN32_
    __os_yield();
    HANDLE timer;
    LARGE_INTEGER ft;

    ft.QuadPart = -(10 * (__int64)usec);

    timer = CreateWaitableTimer(NULL, TRUE, NULL);
    SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0);
    WaitForSingleObject(timer, INFINITE);
    CloseHandle(timer);
//
//__os_yield();
//std::this_thread::sleep_for(std::chrono::microseconds(usec));
#else
    __os_yield();
    usleep(usec);
#endif
}

atomic<bool> threadReleaseBlock = { false };
uInt loggedTime = 0, clocks = 0;
void run_scheduler() {
    fiber *fib = NULL;
    Thread *thread;
    Int size;

    do {
       clocks++;

//        if((clocks % 1000) == 0) {
//            fiber::disposeFibers(); // every ~50ms
//        }

        {
            GUARD(Thread::threadsListMutex)
            size = Thread::threads.size();
        }

        for (Int i = 0; i < size; i++) {
            thread = Thread::threads.at(i);
            loggedTime = NANO_TOMICRO(Clock::realTimeInNSecs());

            if (vm.state >= VM_SHUTTING_DOWN) {
                break;
            }

            if (vm.state != VM_SHUTTING_DOWN && thread->state == THREAD_KILLED) {
                if(!threadReleaseBlock) {
                    GUARD(Thread::threadsListMutex)
                    fiber::killBoundFibers(thread);
                    Thread::destroy(thread);
                    size--;
                }

                continue;
            } else if(thread->state != THREAD_RUNNING)
                continue;

            if (is_thread_ready(thread)) {
                thread->enableContextSwitch(true);
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

        __usleep(CLOCK_CYCLE/2);
    } while(true);
}

bool is_thread_ready(Thread *thread) {
    uInt currentTime = loggedTime;
    if(thread->state != THREAD_RUNNING || hasSignal(thread->signal, tsig_kill) || hasSignal(thread->signal, tsig_suspend))
        return false;

    switch(thread->priority) {
        case THREAD_PRIORITY_LOW:
            return (currentTime - thread->lastRanMicros) > LPTSI;
        case THREAD_PRIORITY_NORM:
            return (currentTime - thread->lastRanMicros) > NPTSI;
        case THREAD_PRIORITY_HIGH:
            return (currentTime - thread->lastRanMicros) > HPTSI;
    }

    return false;
}
