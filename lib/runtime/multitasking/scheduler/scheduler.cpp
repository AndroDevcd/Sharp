//
// Created by bknun on 9/19/2022.
//

#include "scheduler.h"
#include "../../../util/time.h"
#include "../../memory/garbage_collector.h"
#include "../../../core/thread_state.h"

uInt schedTime = 0, clocks = 0, threadCount = 0; // todo: populate this value later as a read only outside of main thread
sched_task *sched_tasks = nullptr, *next_task = nullptr, *last_task = nullptr;
_sched_thread *sched_threads = nullptr, *last_thread = nullptr;
unsched_task *unsched_tasks = nullptr;
_unsched_thread *unsched_threads = nullptr;
bool postIdleTasks = false;

recursive_mutex task_mutex;
recursive_mutex thread_mutex;

void __usleep(unsigned int usec)
{
#ifdef COROUTINE_DEBUGGING
    if(thread_self)
        thread_self->sleepTime += usec;
#endif

    std::this_thread::sleep_for(std::chrono::microseconds(usec));
}

void run_scheduler() {
    _sched_thread *scht;
    uInt sleepTm = 0;

#ifdef WIN32_
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
#endif
#ifdef POSIX_
    pthread_attr_t thAttr;
    int policy = 0;
    int pthread_prio;

    pthread_attr_init(&thAttr);
    pthread_attr_getschedpolicy(&thAttr, &policy);
    pthread_prio = sched_get_priority_max(policy);
    pthread_setschedprio(pthread_self(), pthread_prio);
    pthread_attr_destroy(&thAttr);
#endif

    do {
        clocks++;
        schedTime = NANO_TOMICRO(Clock::realTimeInNSecs());
        sched_unsched_items();
        scht = sched_threads;

        if(next_task != nullptr) {
            while (scht != nullptr) {
                if (scht->thread->id == gc_threadid) {
                    scht = scht->next;
                    continue;
                }

                if (vm.state >= VM_SHUTTING_DOWN) {
                    break;
                }


                scht = sched_thread(scht);
            }
        }

        if(next_task != nullptr)
        {
            if (next_task->next == nullptr) next_task = sched_tasks;
            else next_task = next_task->next;
        } else next_task = sched_tasks;

        if(vm.state >= VM_SHUTTING_DOWN) {
            while(vm.state != VM_TERMINATED) {
                __usleep(1000);
            }

#ifdef COROUTINE_DEBUGGING
            cout << "total sched clocks: " << clocks << endl;
#endif
            return;
        }

        sleepTm = CLOCK_CYCLE - TIME_SINCE;
        if(sleepTm > 0) __usleep(sleepTm);
    } while(true);
}

void sched_unsched_threads() {
    guard_mutex(thread_mutex)

    uInt i = 0;
    while (unsched_threads != nullptr) {
        if ((i++ % 10) == 0 && CLOCKS_SINCE >= 1) {
            return;
        }

        if (!queue_thread(unsched_threads->thread)) {
            return;
        }

        auto *tmp = unsched_threads->next;
        std::free(unsched_threads);
        unsched_threads = tmp;
    }
}

void sched_unsched_tasks() {
    guard_mutex(task_mutex)

    uInt i = 0;
    while (unsched_tasks != nullptr) {
        if ((i++ % 100) == 0 && CLOCKS_SINCE >= 1) {
            return;
        }

        if (!queue_task(unsched_tasks->task)) {
            return;
        }

        auto *tmp = unsched_tasks->next;
        std::free(unsched_tasks);
        unsched_tasks = tmp;
    }
}

void sched_unsched_items() {
    if(unsched_threads) sched_unsched_threads();
    if(unsched_tasks) sched_unsched_tasks();
}

bool is_thread_ready(sharp_thread *thread) {
    if(thread->state != THREAD_RUNNING || hasSignal(thread->signal, tsig_kill)
       || hasSignal(thread->signal, tsig_suspend))
        return false;

    switch(thread->priority) {
        case THREAD_PRIORITY_LOW:
            return (schedTime - thread->lastRanMicros) > LPTSI;
        case THREAD_PRIORITY_NORM:
            return (schedTime - thread->lastRanMicros) > NPTSI;
        case THREAD_PRIORITY_HIGH:
            return (schedTime - thread->lastRanMicros) > HPTSI;
    }

    return false;
}

bool is_runnable(fiber *task, sharp_thread *thread) {
    if(task->state == FIB_SUSPENDED && task->wakeable) {
        return (task->boundThread == thread || task->boundThread == NULL);
    }

    return false;
}

bool can_sched(fiber *task, sharp_thread *thread) {
    return is_runnable(task, thread)
           && (task->acquiringMut == NULL || task->acquiringMut->fiberid == -1)
           &&   (task->delayTime <= 0 || (schedTime /1000L) > task->delayTime);
}

void post(sharp_thread* thread) {
    guard_mutex(thread_mutex)
    _unsched_thread *t = (_unsched_thread*)malloc(sizeof(_unsched_thread));

    if(t)
    {
        t->thread = thread;

        if(unsched_threads == NULL) {
            t->next = NULL;
        } else {
            t->next = unsched_threads;
        }

        unsched_threads = t;
    }
}

void post(fiber* fib) {
    guard_mutex(task_mutex)
    unsched_task *t = (unsched_task*)malloc(sizeof(unsched_task));

    if(t)
    {
        t->task = fib;

        if(unsched_tasks == NULL) {
            t->next = NULL;
        } else {
            t->next = unsched_tasks;
        }

        unsched_tasks = t;
    }
}

bool queue_task(fiber *fib) {
    sched_task *task = (sched_task*)malloc(sizeof(sched_task));

    if(task)
    {
        if(sched_tasks == NULL) {
            sched_tasks = task;
        } else {
            last_task->next = task;
        }

        task->prev = last_task;
        task->next = NULL;
        task->task = fib;
        last_task = task;
        return true;
    }

    return false;
}

bool queue_thread(sharp_thread* thread) {
    _sched_thread *t = (_sched_thread*)malloc(sizeof(_sched_thread));

    if(t)
    {
        if(sched_threads == NULL) {
            sched_threads = t;
        } else {
            last_thread->next = t;
        }

        t->prev = last_thread;
        t->next = NULL;
        t->thread = thread;
        last_thread = t;
        return true;
    }

    return false;
}

void dispose(sched_task *task) {
    guard_mutex(task_mutex)
    free_struct(task->task);

    if(task == last_task)
        last_task = task->prev;
    if(task == sched_tasks)
        sched_tasks = task->next;
    if(task->prev != NULL)
        task->prev->next = task->next;
    if(task->next != NULL)
        task->next->prev = task->prev;

    std::free(task->task);
    std::free(task);
}

void sched(sharp_thread *thread) {
    sched_task *starting_task = next_task;
    sched_task *end_task = NULL, *task = starting_task;
    bool wrapped = false;

    while(task != end_task) {
        if (vm.state >= VM_SHUTTING_DOWN) {
            break;
        }

        if(can_sched(task->task, thread)) {
            return post_task(thread, task->task);
        }

        task = task->next;
        if(task == NULL && !wrapped) {
            wrapped = true;
            end_task = starting_task->next;
            task = sched_tasks;
        }
    }
}
