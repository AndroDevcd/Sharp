//
// Created by bnunnally on 8/19/21.
//

#include "../../../stdimports.h"
#include "idle_scheduler.h"
#include "thread_controller.h"
#include "../Thread.h"
#include "scheduler.h"
#include "thread_controller.h"
#include "../linked_list.h"
#include "../VirtualMachine.h"

sched_task *idle_tasks, *last_idle_task;
Thread *idleSched = NULL;
recursive_mutex idle_mutex;
_List<sched_task*> unAddedIdleTasks;

void start_idle_scheduler() {
    if(idleSched) {
        start_daemon_thread(
                idle_sched_main,
                idleSched
        );
    }
}

void create_idle_scheduler() {
    Thread* idle_sched = (Thread*)malloc(
            sizeof(Thread)*1);

    string threadName = "Idle Scheduler";
    uInt threadId = generate_thread_id();
    if(threadId == ILL_THREAD_ID)
        return; // unlikely

    idleSched = idle_sched;
    initialize(idle_sched, threadName, threadId, true, NULL, false);
    idle_sched->priority = THREAD_PRIORITY_LOW;

    post(idle_sched);
}

#ifdef WIN32_
DWORD WINAPI
#endif
#ifdef POSIX_
void*
#endif
idle_sched_main(void *pVoid) {
    thread_self = (Thread*)pVoid;

    try {
        run_idle_scheduler();
    } catch(Exception &e){
        /* Should never happen */
        sendSignal(thread_self->signal, tsig_except, 1);
    }

    shutdown_thread(thread_self);

#ifdef WIN32_
    return 0;
#endif
#ifdef POSIX_
    return nullptr;
#endif
}

void run_idle_scheduler() {
    fiber *fib = NULL;

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
        if(hasSignal(idleSched->signal, tsig_suspend))
            suspend_self();
        if(idleSched->state == THREAD_KILLED || hasSignal(idleSched->signal, tsig_kill)) {
            return;
        }

        release_tasks();
        add_idle_tasks();

        if(vm.state >= VM_SHUTTING_DOWN) {
            while(vm.state != VM_TERMINATED) {
                __usleep(1000);
            }

#ifdef COROUTINE_DEBUGGING
            cout << "total sched clocks: " << clocks << endl;
#endif
            return;
        }

        __usleep(IDLE_SCHED_CLOCK_CYCLE);
    } while(true);
}

void post_idle_task(sched_task *task) {
    GUARD(idle_mutex)
    unAddedIdleTasks.add(task);
}

bool is_idle_task(sched_task *task) {
    return task->task->state == FIB_SUSPENDED &&
        (
            (task->task->delayTime > 0 && ((schedTime - task->task->delayTime) >= MIN_DELAY_TIME + 1))
            || !task->task->wakeable
            || task->task->acquiringMut != NULL
        );
}

void queue_idle_task(sched_task* task) {
    if(idle_tasks == NULL) {
        idle_tasks = task;
    } else {
        last_idle_task->next = task;
    }

    task->prev = last_idle_task;
    task->next = NULL;
    last_idle_task = task;
}

void add_idle_tasks() {
    if(unAddedIdleTasks.size() > 0) {
        GUARD(idle_mutex)
        Int tasks = unAddedIdleTasks.size();

        for(uInt i = 0; i < tasks; i++) {
            queue_idle_task(unAddedIdleTasks.get(i));
        }

        unAddedIdleTasks.free();
    }
}

bool can_release_idle_task(sched_task *task) {
    return (
            task->task->state != FIB_SUSPENDED
         || task->task->wakeable
         || (task->task->acquiringMut != NULL
            && task->task->acquiringMut->fiberid == -1
            && task->task->acquiringMut->threadid == -1)
         || (task->task->delayTime > 0 && ((schedTime - task->task->delayTime) <= 0))
    );
}

fiber* locate_idle_task(uInt taskId) {
    GUARD(idle_mutex)
    sched_task *task = idle_tasks;

    while(task != NULL) {
        if(task->task->id == taskId)
            return task->task;

        task = task->next;
    }

    auto *unsched_t = unsched_tasks;
    while(unsched_t == NULL) {
        if(unsched_t->task->id == taskId)
            return unsched_t->task;

        unsched_t = unsched_t->next;
    }

    return NULL;
}

void release_idle_task(sched_task *task) {
    post(task->task);

    if(task == last_idle_task)
        last_idle_task = task->prev;
    if(task == idle_tasks)
        idle_tasks = task->next;
    if(task->prev != NULL)
        task->prev->next = task->next;
    if(task->next != NULL)
        task->next->prev = task->prev;

    std::free(task);
}

sched_task *task, *next_idle_task;
Int releasedTasks = 0;
void release_tasks() {
    releasedTasks = 0;
    task = idle_tasks;

    while(task != NULL)
    {
        next_idle_task = task->next;
        if(can_release_idle_task(task)) {
            releasedTasks++;
            release_idle_task(task);
        }

        if (releasedTasks >= MAX_TASK_RELEASE
            || vm.state >= VM_SHUTTING_DOWN) {
            break;
        }

        task = next_idle_task;
    }
}
