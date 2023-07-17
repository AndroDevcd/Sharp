//
// Created by bnunnally on 8/17/21.
//

#include "task_controller.h"
#include "../scheduler/scheduler.h"
#include "fiber.h"
#include "../../memory/memory_helpers.h"
#include "../../error/vm_exception.h"
#include "../../memory/garbage_collector.h"
#include "../../../core/thread_state.h"
#include "../thread/sharp_thread.h"
#include "../thread/thread_controller.h"
#include "../../../util/time.h"

atomic<uInt> taskIds = { 0 };
fiber* create_task(string &name, sharp_function *main) {
    fiber *fib = nullptr;

    if(name.find("worker_thread") != string::npos) {
        int i = 0;
    }
    try {
        fib = malloc_struct<fiber>(sizeof(fiber), 1);
        fib->name = name;
        fib->main = main;
        fib->id = taskIds++;
        post(fib);
    } catch(vm_exception &e) {
        if(fib) {
            fib->free();
            std::free(fib);
        }
        throw;
    }

    return fib;
}

void delay_task(uInt time) {
    if(time <= 0)
        time = NO_DELAY;

    sharp_thread *thread = thread_self;
    fiber *fib = thread->task;
    set_task_state(thread, fib, FIB_SUSPENDED, time);
    enable_context_switch(thread, true);
}

void set_attached_thread(fiber* task, sharp_thread* thread) {
    guard_mutex(task->mut)
    task->attachedThread = thread;
}

sharp_thread* get_bound_thread(fiber* task) {
    guard_mutex(task->mut);
    return task->boundThread;
}

sharp_thread* get_attached_thread(fiber* task) {
    guard_mutex(task->mut);
    return task->attachedThread;
}

Int get_state(fiber* task) {
    guard_mutex(task->mut)
    return (Int)task->state;
}

bool is_bound(fiber *fib, sharp_thread *thread) {
    return fib->boundThread == thread && fib->state != FIB_KILLED;
}

Int bind_task(fiber* task, sharp_thread* thread) {
    guard_mutex(task->mut)

    if(thread != nullptr) {
        if(thread->state != THREAD_KILLED && !hasSignal(thread->signal, tsig_kill)) {
            task->boundThread = thread;
            thread->boundFibers++;
            return 0;
        }
    } else {
        if(task->boundThread) {
            task->boundThread->boundFibers--;
        }

        task->boundThread = NULL;
        return 0;
    }

    return 1;
}

void set_task_state(sharp_thread* thread, fiber* task, fiber_state newState, uInt delay) {
    guard_mutex(task->mut)

    switch(newState) {
        case FIB_CREATED:
            task->state = newState;
            task->delayTime = -1;
            break;
        case FIB_RUNNING:
            if(thread) thread->lastRanMicros = NANO_TOMICRO(Clock::realTimeInNSecs());
            task->state = newState;
            task->delayTime = -1;
            break;
        case FIB_SUSPENDED: {
            if(task->state == FIB_KILLED) return;
            if(delay > 0) {
                task->delayTime = NANO_TOMILL(Clock::realTimeInNSecs()) + delay;
            } else task->delayTime = -1;
            task->state = newState;
            break;
        }
        case FIB_KILLED:
            bind_task(task, NULL);
            task->state = newState;
            task->finished = true;
            task->delayTime = -1;
            break;
    }
}

void set_task_wakeable(fiber* task, bool enable) {
    guard_mutex(task->mut)
    task->wakeable = enable;
}

Int unsuspend_task(uInt id) {
    fiber *fib = locate_task(id);
    int result = 0;

    if(fib) {
        guard_mutex(fib->mut)
        if(fib->state == FIB_SUSPENDED) {
            fib->delayTime = -1;
            set_task_wakeable(fib, true);
        } else {
            result = 1;
        }

    }

    return result;
}

Int suspend_task(uInt id) {
    fiber *fib = locate_task(id);
    int result = 0;

    if(fib) {
        guard_mutex(fib->mut)
        if(fib->state == FIB_RUNNING) {
            if(fib->attachedThread) {
                set_task_wakeable(fib, false);
                set_task_state(fib->attachedThread, fib, FIB_SUSPENDED, NO_DELAY);
                enable_context_switch(fib->attachedThread, true);
            } else {
                result = 2;
            }
        } else if(fib->state == FIB_SUSPENDED) {
            if(fib->attachedThread == nullptr) {
                set_task_wakeable(fib, false);
            } else {
                result = 2;
            }
        } else {
            result = 1;
        }

    }

    return result;
}

int kill_task(uInt taskId) {
    fiber *task = locate_task(taskId);
    if(task != NULL) return kill_task(task);
    else return 0;
}

int kill_task(fiber *fib) {
    int result = 0;

    if(fib) {
        if(fib->state == FIB_SUSPENDED) {
            set_task_state(NULL, fib, FIB_KILLED, NO_DELAY);
        } else if(fib->state == FIB_RUNNING) {
            if(fib->attachedThread) {
                set_task_state(NULL, fib, FIB_KILLED, NO_DELAY);
                enable_context_switch(fib->attachedThread, true);
            } else {
                result = 1;
            }
        } else {
            set_task_state(NULL, fib, FIB_KILLED, NO_DELAY);
        }

    }

    return result;
}

