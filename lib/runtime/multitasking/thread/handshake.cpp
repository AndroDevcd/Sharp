//
// Created by bknun on 9/21/2022.
//

#include "handshake.h"
#include "../../../util/time.h"
#include "../scheduler/scheduler.h"
#include "../../virtual_machine.h"
#include "../../memory/garbage_collector.h"
#include "sharp_thread.h"
#include "../../../core/thread_state.h"
#include "thread_controller.h"

void init_struct(handshake *hs) {
    hs->listening = false;
    hs->accepted = false;
    hs->data = nullptr;
    hs->code = 0;
    new (&hs->mut) recursive_mutex();
}

KeyPair<Int, void*> listen(handshake *hs, uInt usTime) {
    const int sMaxRetries = 10000;
    int retryCount = 0;

    sharp_thread *thread = thread_self;
    uInt future = NANO_TOMICRO(Clock::realTimeInNSecs()) + usTime;
    hs->data = nullptr;
    hs->code = 0;
    hs->accepted = false;
    hs->listening = true;

    while (hs->data == nullptr)
    {
        if(usTime != -1 && NANO_TOMICRO(Clock::realTimeInNSecs()) > future)
        {
            hs->listening = false;
            return KeyPair<Int, void*>(0, NULL);
        } else if (retryCount++ == sMaxRetries)
        {
            retryCount = 0;
            if(thread != NULL &&
               (thread->state == THREAD_KILLED
                || hasSignal(thread->signal, tsig_kill)
                || hasSignal(thread->signal, tsig_suspend))) {

                if(hasSignal(thread->signal, tsig_suspend)) {
                    suspend_self();
                } else {
                    return KeyPair<Int, void*>(0, NULL);
                }
            } else if (vm.state >= VM_SHUTTING_DOWN) {
                hs->listening = false;
                return KeyPair<Int, void*>(0, NULL);
            }

            __usleep(1);
        }
    }

    hs->listening = false;
    KeyPair<Int, void*> data(hs->code, hs->data);
    hs->accepted = true;

    return data;
}

bool try_handshake(handshake *hs, void *data, Int code, uInt usTime) {
    guard_mutex(hs->mut);

    sharp_thread *thread = thread_self;
    const int sMaxRetries = 10000;
    int retryCount = 0;
    uInt future = NANO_TOMICRO(Clock::realTimeInNSecs()) + usTime;

    while (!hs->listening)
    {
        if (retryCount++ == sMaxRetries)
        {
            if(usTime != -1 && NANO_TOMICRO(Clock::realTimeInNSecs()) > future)
            {
                return false;
            }

            retryCount = 0;
            if(thread != NULL &&
                (thread->state == THREAD_KILLED
                || hasSignal(thread->signal, tsig_kill)
                || hasSignal(thread->signal, tsig_suspend))) {

                if(hasSignal(thread->signal, tsig_suspend)) {
                    suspend_self();
                } else {
                    return false;
                }
            }
            else if (vm.state >= VM_SHUTTING_DOWN) {
                return false;
            }

            __usleep(1);
        }
    }

    hs->data = data;
    hs->code = code;
    future = NANO_TOMICRO(Clock::realTimeInNSecs()) + 1000;
    while (hs->listening && !hs->accepted)
    {
        if (retryCount++ == sMaxRetries)
        {
            if(usTime != -1 && NANO_TOMICRO(Clock::realTimeInNSecs()) > future)
            {
                hs->data = nullptr;
                return hs->accepted;
            }

            retryCount = 0;
            if (vm.state >= VM_SHUTTING_DOWN
                || thread->state == THREAD_KILLED
                || hasSignal(thread->signal, tsig_kill)) {
                hs->data = nullptr;
                return hs->accepted;
            }

            __usleep(1);
        }
    }

    return hs->accepted;
}
