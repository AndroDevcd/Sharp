//
// Created by BNunnally on 9/5/2020.
//

#include "../../../stdimports.h"
#include "scheduler.h"
#include "thread_controller.h"
#include "idle_scheduler.h"
#include "../Thread.h"
#include "../linked_list.h"
#include "../VirtualMachine.h"

void __usleep(unsigned int usec)
{
#ifdef COROUTINE_DEBUGGING
    if(thread_self)
        thread_self->actualSleepTime += usec;
#endif

#ifdef WIN32_
    HANDLE timer;
    LARGE_INTEGER ft;

    ft.QuadPart = -(10 * (__int64)usec);

    timer = CreateWaitableTimer(NULL, TRUE, NULL);
    SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0);
    WaitForSingleObject(timer, INFINITE);
    CloseHandle(timer);
#else
    __os_yield()
    usleep(usec);
#endif
}

uInt schedTime = 0, clocks = 0, schedTasks = 0;
sched_task *sched_tasks = NULL, *next_task = NULL, *last_task = NULL;
_sched_thread *sched_threads = NULL, *last_thread = NULL;

recursive_mutex task_mutex;
recursive_mutex thread_mutex;

_List<Thread*> unSchedThreads;
_List<fiber*> unSchedTasks;

void run_scheduler() {
    fiber *fib = NULL;
    _sched_thread *thread;
    Int size;

#ifdef WIN32_
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);
#endif
#ifdef POSIX_
    pthread_attr_t thAttr;
    int policy = 0;
    int pthread_prio;

    pthread_attr_init(&thAttr);
    pthread_attr_getschedpolicy(&thAttr, &policy);
    pthread_prio = sched_get_priority_min(policy);
    pthread_setschedprio(pthread_self(), pthread_prio);
    pthread_attr_destroy(&thAttr);
#endif

    do {
       clocks++;
       if(clocks % 1000 == 0)
       {
           clear_tasks();
       }

       sched_unsched_items();
       schedTime = NANO_TOMICRO(Clock::realTimeInNSecs());
       thread = sched_threads;

       if(next_task->next == NULL) next_task = sched_tasks;
       else next_task = next_task->next;

       while(thread != NULL)
       {
           if(thread->thread->id == gc_threadid) continue;

           if (vm.state >= VM_SHUTTING_DOWN) {
               break;
           }

           thread = sched_thread(thread);
       }

       // test normal scheduling first
       // send off idle tasks ~ every millisecond
//       if(clocks % 4 == 0) {
//           post_idle_tasks();
//       }

       if(vm.state >= VM_SHUTTING_DOWN) {
           while(vm.state != VM_TERMINATED) {
               __usleep(1000);
           }

#ifdef COROUTINE_DEBUGGING
           cout << "total sched clocks: " << clocks << endl;
#endif
           return;
       }

        __usleep(CLOCK_CYCLE);
    } while(true);
}

void post_idle_tasks() {
    Int postedTasks = 0;
    sched_task *task = sched_tasks, *next;
    if(idleSched == NULL) return;

    while(task != NULL) {
        if (vm.state >= VM_SHUTTING_DOWN) {
            break;
        }

        next = task->next;
        if(is_idle_task(task)) {
            GUARD(task_mutex)

            if(task == last_task)
                last_task = task->prev;
            if(task->prev != NULL)
                task->prev->next = task->next;
            if(task->next != NULL)
                task->next->prev = task->prev;

            postedTasks++;
            post_idle_task(task);
        }

        task = next;
        if(postedTasks >= MAX_TASK_RELEASE)
            break;
    }
}

bool is_thread_ready(Thread *thread) {
    uInt currentTime = schedTime;
    if(thread->state != THREAD_RUNNING || hasSignal(thread->signal, tsig_kill)
    || hasSignal(thread->signal, tsig_suspend))
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

bool is_runnable(fiber *task, Thread *thread) {
    if(task->state == FIB_SUSPENDED && task->wakeable) {
        return (task->boundThread == thread || task->boundThread == NULL);
    }

    return false;
}

bool can_sched(fiber *task, Thread *thread) {
    return is_runnable(task, thread) &&
           (task->delayTime == 0 || schedTime > task->delayTime);
}

bool can_dispose(sched_task *task) {
    fiber *fib = task->task;
    return fib->finished && fib->state == FIB_KILLED && fib->attachedThread == NULL;
}

void post(Thread* thread) {
    GUARD(thread_mutex)
    unSchedThreads.add(thread);
}

void post(fiber* fib) {
    GUARD(task_mutex)
    unSchedTasks.add(fib);
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

bool queue_thread(Thread* thread) {
    _sched_thread *t = (_sched_thread*)malloc(sizeof(_sched_thread));

    if(t)
    {
        if(sched_tasks == NULL) {
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

void sched_unsched_items() {
    const Int maxTasks = 10000;
    if(unSchedThreads.size() > 0) {
        GUARD(thread_mutex)
        Int tasks = unSchedThreads.size() % maxTasks;
        if(tasks == 0) tasks = maxTasks-1;

        for(uInt i = 0; i < tasks; i++) {
            if(!queue_thread(unSchedThreads.get(i))) {
                unSchedThreads.removeUntil(i);
                return;
            }
        }

        if(unSchedThreads.size() > maxTasks) {
            unSchedThreads.removeUntil(maxTasks);
        } else
            unSchedThreads.free();
    }

    if(unSchedTasks.size() > 0) {
        GUARD(task_mutex)
        Int tasks = unSchedTasks.size() % maxTasks;
        if(tasks == 0) tasks = maxTasks-1;

        for(uInt i = 0; i < tasks; i++) {
            if(!queue_task(unSchedTasks.get(i))) {
                unSchedTasks.removeUntil(i);
                return;
            }
        }

        if(unSchedTasks.size() > maxTasks) {
            unSchedTasks.removeUntil(maxTasks);
        } else
            unSchedTasks.free();
    }
}

void dispose(sched_task *task) {
    GUARD(task_mutex)
    task->task->free();

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

void clear_tasks() {
    sched_task *task = sched_tasks;
    sched_task *disposable = NULL;
    GUARD(idle_mutex)

    while(task != NULL) {

        if(can_dispose(task))
        {
            disposable = task;
        }

        task = task->next;
        if(disposable != NULL) {
            dispose(disposable);
            disposable = NULL;
        }
    }
}

void sched(Thread *thread) {
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
