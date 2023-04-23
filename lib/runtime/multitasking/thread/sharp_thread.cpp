//
// Created by bknun on 9/16/2022.
//

#include "sharp_thread.h"
#include "../../../core/thread_state.h"
#include "thread_controller.h"


bool foo() {}

void init_struct(sharp_thread *thread) {
    thread->scht = nullptr;
    new(&thread->boundFibers) atomic<Int>();
    new (&thread->mut) recursive_mutex();
    thread->id = 0;
    thread->stackSize = 0;
    thread->stbase = 0;
    thread->stfloor = 0;
    thread->priority = THREAD_PRIORITY_NORM;
    thread->daemon = false;
    thread->terminated = false;
    thread->state = 0;
    thread->signal = 0;
    thread->suspended = false;
    thread->exited = false;
    thread->marked = false;
    new (&thread->name) string();
    init_struct(&thread->currentThread);
    init_struct(&thread->args);
    thread->mainMethod = nullptr;
    thread->lastRanMicros = 0;
    thread->thread = nullptr;
    init_struct(&thread->queueNotification);
    thread->queueNotification.cond = queue_filled;
#ifdef COROUTINE_DEBUGGING
    thread->timeSleeping = 0;
    thread->switched = 0;
    thread->skipped = 0;
    thread->actualSleepTime = 0;
    thread->contextSwitchTime = 0;
    thread->timeLocking = 0;
#endif
}
