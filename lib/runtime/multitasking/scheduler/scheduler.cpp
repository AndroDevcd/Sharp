//
// Created by bknun on 9/19/2022.
//

#include "scheduler.h"
#include "../../../util/time.h"
#include "../../memory/garbage_collector.h"
#include "../../../core/thread_state.h"
#include "../../multitasking/thread/sharp_thread.h"
#include "../../virtual_machine.h"
#include "../thread/thread_controller.h"
#include "../../multitasking/fiber/fiber.h"
#include "idle_scheduler.h"
#include "../fiber/task_controller.h"

uInt schedTime = 0, clocks = 0;
uInt taskCount = 0; /* Read-Only */

// internal
sched_task *sched_tasks = nullptr;
_sched_thread *sched_threads = nullptr;

// external
unsched_task *unsched_tasks = nullptr;
_unsched_thread *unsched_threads = nullptr;

recursive_mutex task_mutex;
recursive_mutex thread_mutex;

void __usleep(unsigned int usec)
{
#ifdef COROUTINE_DEBUGGING
    if(thread_self)
        thread_self->timeSleeping += usec;
#endif

    std::this_thread::sleep_for(std::chrono::microseconds(usec));
}

void run_scheduler() {
    _sched_thread *schth = nullptr;
    sched_task *scht = nullptr;
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
        schth = sched_threads;

//        if(taskCount == 1) {
//            this_thread::yield();
//            goto check_state;
//        }

        if(scht == nullptr) {
            scht = sched_tasks;
        }

        // prepare threads for context switch
        while (schth != nullptr) {
            if (schth->thread->id == gc_threadid || schth->thread->id == idle_threadid) {
                schth = schth->next;
                continue;
            }

            if (vm.state >= VM_SHUTTING_DOWN) {
                break;
            }

            auto next = schth->next;
            thread_sched_prepare(schth);
            schth = next;
        }

        std::this_thread::yield(); // yield to give threads some time to go into sched mode
        schth = sched_threads;
        while (schth != nullptr) {
            if(!can_sched_thread(schth->thread)) {
                schth = schth->next;
                continue;
            }

            if(scht == nullptr) {
                scht = sched_tasks;
            }

            bool taskWrap = false;
            while (scht != nullptr) {
                if (vm.state >= VM_SHUTTING_DOWN) {
                    break;
                }

                if(can_purge(scht->task)) {
                    auto next = scht->next;
                    dispose(scht);
                    scht = next;
                    goto wrap;
                }
                else if(taskCount > MAIN_POOL_SIZE_LIMIT && is_task_idle(scht)) {
                    auto next = scht->next;
                    post_idle_task(scht);
                    remove_task(scht);
                    scht = next;
                    goto wrap;
                }
                else if(is_runnable(scht->task, schth->thread)) {
                    queue_task(schth->thread, scht);
                    break;
                }

                scht = scht->next;

                wrap:
                if(scht == NULL && !taskWrap) {
                    taskWrap = true;
                    scht = sched_tasks;
                }
            }

            schth = schth->next;
        }

        check_state:
        if(vm.state >= VM_SHUTTING_DOWN) {
            while(vm.state != VM_TERMINATED) {
                __usleep(1000);
            }

#ifdef COROUTINE_DEBUGGING
            cout << "total sched clocks: " << clocks << endl;
#endif
            return;
        }

        std::this_thread::yield();
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
        if ((i++ % 10) == 0 && CLOCKS_SINCE >= 1) {
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
    if(task->attachedThread == NULL && (task->state == FIB_SUSPENDED || task->state == FIB_CREATED) && task->wakeable) {
        if(task->delayTime != -1 && NANO_TOMILL(Clock::realTimeInNSecs()) < task->delayTime) {
            return false;
        }

        return (task->f_lock == NULL || (task->f_lock->id == -1)) &&
            (task->boundThread == thread || task->boundThread == NULL);
    }

    return false;
}

bool can_purge(fiber *task) {
    return task->state == FIB_KILLED && task->attachedThread == nullptr;
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
        task->prev = NULL;
        task->task = fib;

        if(sched_tasks == NULL) {
            sched_tasks = task;
            task->next = NULL;
        } else {
            sched_tasks->prev = task;
            task->next = sched_tasks;
            sched_tasks = task;
        }

        taskCount++;
        return true;
    }

    return false;
}

_sched_thread* get_sched_threads() {
    return sched_threads;
}

bool queue_thread(sharp_thread* thread) {
    _sched_thread *t = (_sched_thread*)malloc(sizeof(_sched_thread));

    if(t)
    {
        t->prev = NULL;
        t->thread = thread;

        if(sched_threads == NULL) {
            sched_threads = t;
            t->next = NULL;
        } else {
            sched_threads->prev = t;
            t->next = sched_threads;
            sched_threads = t;
        }

        return true;
    }

    return false;
}

void remove_task(sched_task *task) {
    guard_mutex(task_mutex)

    if(task == sched_tasks)
        sched_tasks = task->next;

    if(task->next == NULL) {
        if(task->prev != NULL) {
            task->prev->next = NULL;
        } else {
            sched_tasks = NULL;
        }
    } else if(task->prev == NULL) {
        if(task->next != NULL) {
            task->next->prev = NULL;
        } else {
            sched_tasks = NULL;
        }
    } else {
        task->prev->next = task->next;
        task->next->prev = task->prev;
    }

    taskCount--;
    std::free(task);
}

void dispose(sched_task *task) {
    guard_mutex(task_mutex)
    task->task->free();
    std::free(task->task);
    remove_task(task);
}


fiber *locate_task(uInt taskId) {
    guard_mutex(task_mutex)
    sched_task *task = sched_tasks;

    while(task != NULL) {
        if(task->task->id == taskId)
            return task->task;

        task = task->next;
    }

    return nullptr;
}

void kill_bound_tasks(sharp_thread* thread) {
    guard_mutex(task_mutex)
    sched_task *task = sched_tasks, *next;

    while (task != NULL) {
        next = task->next;

        if (is_bound(task->task, thread)) {
            set_task_state(NULL, task->task, FIB_KILLED, NO_DELAY);
        }

        task = next;
    }
}

void dispose(_sched_thread *scht) {
    guard_mutex(thread_mutex)
    sharp_thread *thread = scht->thread;
    if (thread->state == THREAD_KILLED || thread->terminated)
    {
        if(scht == sched_threads)
            sched_threads = scht->next;

        if(scht->next == NULL) {
            if(scht->prev != NULL) {
                scht->prev->next = NULL;
            } else {
                sched_threads = NULL;
            }
        } else if(scht->prev == NULL) {
            if(scht->next != NULL) {
                scht->next->prev = NULL;
            } else {
                sched_threads = NULL;
            }
        } else {
            scht->prev->next = scht->next;
            scht->next->prev = scht->prev;
        }

        free_struct(thread);
        std::free (thread);
        std::free (scht);
    }
}

