//
// Created by bknun on 9/16/2022.
//

#include "sharp_thread.h"
#include "../../../core/thread_state.h"
#include "thread_controller.h"
#include "../../memory/garbage_collector.h"


void free_struct(sharp_thread *thread) {
    // do nothing
    release_bytes(sizeof(sharp_thread));
    copy_object(&thread->currentThread, (sharp_object*) nullptr);
    copy_object(&thread->args, (sharp_object*) nullptr);
}

void init_struct(sharp_thread *thread) {
    thread->task = nullptr;
    new(&thread->boundFibers) atomic<Int>();
    new(&thread->queue) atomic<sched_task*>();
    new (&thread->mut) recursive_mutex();
    thread->id = 0;
    thread->stackSize = 0;
    thread->priority = THREAD_PRIORITY_NORM;
    thread->daemon = false;
    thread->terminated = false;
    thread->state = 0;
    thread->nativeCalls = 0;
    thread->signal = 0;
    thread->suspended = false;
    thread->exited = false;
    thread->marked = false;
    new (&thread->name) string();
    init_struct(&thread->currentThread);
    init_struct(&thread->args);
    thread->mainMethod = nullptr;
    thread->lastRanMicros = 0;
    thread->thread = 0;
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
