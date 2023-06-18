//
// Created by bknun on 4/22/2023.
//

#include "idle_scheduler.h"
#include "../thread/thread_controller.h"
#include "../thread/sharp_thread.h"
#include "../../../core/thread_state.h"
#include "../../error/vm_exception.h"
#include "../../memory/garbage_collector.h"
#include "../../../util/time.h"
#include "../fiber/fiber.h"
#include "../fiber/task_controller.h"

unsched_task *idle_pool = nullptr;
std::recursive_mutex idle_mutex;
uInt idleTime = 0;

bool should_post_task(unsched_task *task) {
    return taskCount < MAIN_POOL_SIZE_LIMIT
        && task->task->wakeable
        && (NANO_TOMILL(Clock::realTimeInNSecs()) - task->task->delayTime) > 0
        && (task->task->f_lock == nullptr || task->task->f_lock->id == -1);
}

bool is_task_idle(sched_task *task) {
    return task->task->state == FIB_SUSPENDED && task->task->attachedThread == nullptr &&
            (
                !task->task->wakeable
                || (NANO_TOMILL(Clock::realTimeInNSecs()) - task->task->delayTime) < 0
                || (task->task->f_lock != nullptr && task->task->f_lock->id != -1)
            );
}

void kill_bound_idle_tasks(sharp_thread* thread) {
    guard_mutex(task_mutex)
    unsched_task *task = idle_pool, *next;

    while (task != NULL) {
        next = task->next;

        if (is_bound(task->task, thread)) {
            set_task_state(NULL, task->task, FIB_KILLED, NO_DELAY);
        }

        task = next;
    }
}

void post_idle_task(sched_task *task) {
    guard_mutex(idle_mutex);
    unsched_task *t = (unsched_task*)malloc(sizeof(unsched_task));

    if(t)
    {
        t->task = task->task;

        if(idle_pool == NULL) {
            t->next = NULL;
        } else {
            t->next = idle_pool;
        }

        idle_pool = t;
    }
}

void idle_main_loop() {
    for(;;) {
        check_state:
        CHECK_STATE

        do {
            idleTime = NANO_TOMICRO(Clock::realTimeInNSecs());
            unsched_task *idleTask = idle_pool, *prevTask = nullptr;
            uInt index = 0;

            while(idleTask != nullptr) {
//                if((index % 1000) == 0 && TIME_SINCE(idleTime) > (CLOCK_CYCLE * 2)) { // sleep every clock cycle
////                    __usleep(CLOCK_CYCLE * 2);
//
//                    std::this_thread::yield();
//                }
                if(can_purge(idleTask->task)) {
                    idleTask->task->free();
                    auto nextTask = idleTask->next;
                    if(idleTask == idle_pool)
                        idle_pool = idleTask->next;
                    else if(prevTask != nullptr)
                        prevTask->next = idleTask->next;

                    std::free(idleTask->task);
                    std::free(idleTask);
                    idleTask = nextTask;
                } else if(should_post_task(idleTask)) {
                    guard_mutex(idle_mutex);
                    auto task = idleTask->task;
                    auto nextTask = idleTask->next;
                    if(idleTask == idle_pool)
                        idle_pool = idleTask->next;
                    else if(prevTask != nullptr)
                        prevTask->next = idleTask->next;

                    std::free(idleTask);
                    idleTask = nextTask;
                    post(task);
                } else {
                    prevTask = idleTask;
                    idleTask = idleTask->next;
                }

                index++;
            }

            if(thread_self->signal)
                goto check_state;
            std::this_thread::yield();
        }  while(true);
    }
}

#ifdef WIN32_
DWORD WINAPI
#endif
#ifdef POSIX_
void*
#endif
idle_main(void *pVoid) {
    thread_self = (sharp_thread*)pVoid;
    thread_self->state = THREAD_RUNNING;
    set_thread_priority(thread_self, THREAD_PRIORITY_LOW);

    try {
        idle_main_loop();
    } catch(vm_exception &e){
        /* Should never happen */
        enable_exception_flag(thread_self, true);
    }

    /*
     * Check for uncaught exception in thread before exit
     */
    shutdown_thread(thread_self);
#ifdef WIN32_
    return 0;
#endif
#ifdef POSIX_
    return nullptr;
#endif
}

void idle_handler_startup() {
    start_daemon_thread(
            idle_main,
            create_idle_handler_thread()
    );
}