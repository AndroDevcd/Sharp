//
// Created by bknun on 9/16/2022.
//

#include "../../sni/sni_runtime.h"
#include "sharp_thread.h"
#include "../thread_state.h"
#include "thread_controller.h"
#include "../../memory/garbage_collector.h"
#include "../../memory/tracing_collector.h"
#include "../../memory/sharp_object.h"


void free_struct(sharp_thread *thread) {
    tgc_unregister_thread(thread); // the collector must not look at it any more
    pool_give_back(thread->pool);  // its free blocks are for the other threads now
    sni_free_thread_state(thread);
    release_bytes(sizeof(sharp_thread));
    copy_object(&thread->currentThread, (sharp_object*) nullptr);
    copy_object(&thread->args, (sharp_object*) nullptr);
}

void init_struct(sharp_thread *thread) {
    thread->task = nullptr;
    new(&thread->boundFibers) atomic<Int>();
    new(&thread->queue) atomic<fiber*>(nullptr);
    new (&thread->mut) recursive_mutex();
    thread->id = 0;
    thread->stackSize = 0;
    thread->nativeStackFloor = 0;
    thread->sni = nullptr;
    thread->priority = THREAD_PRIORITY_NORM;
    thread->daemon = false;
    thread->terminated = false;
    thread->state = 0;
    thread->nativeCalls = 0;
    thread->signal = 0;
    thread->suspended = false;
    thread->exited = false;
    thread->marked = false;
    thread->users = 0;
    thread->youngObjects = nullptr;
    thread->uncountedBytes = 0;
    for(int c = 0; c < SHARP_POOL_LISTS; c++) thread->pool[c] = nullptr;
    new (&thread->name) string();
    init_struct(&thread->currentThread);
    init_struct(&thread->args);
    thread->lastRanMicros = 0;
    thread->readyHead = nullptr;
    thread->readyTail = nullptr;
    thread->parked = false;
    thread->pickCount = 0;
    thread->readyHead = nullptr;
    thread->readyTail = nullptr;
    thread->parked = false;
    thread->pickCount = 0;
    thread->readyHead = nullptr;
    thread->readyTail = nullptr;
    thread->parked = false;
    thread->pickCount = 0;
    thread->readyHead = nullptr;
    thread->readyTail = nullptr;
    thread->parked = false;
    thread->pickCount = 0;
    thread->readyHead = nullptr;
    thread->readyTail = nullptr;
    thread->parked = false;
    thread->pickCount = 0;
    thread->thread = 0;
    init_struct(&thread->queueNotification);
    thread->queueNotification.cond = queue_filled;
    tgc_register_thread(thread);   // from now on the collector knows about it (starts out "safe")
#ifdef COROUTINE_DEBUGGING
    thread->timeSleeping = 0;
    thread->switched = 0;
    thread->skipped = 0;
    thread->actualSleepTime = 0;
    thread->contextSwitchTime = 0;
    thread->timeLocking = 0;
#endif
}
