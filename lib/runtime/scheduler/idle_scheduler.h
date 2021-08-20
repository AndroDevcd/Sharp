//
// Created by bnunnally on 8/19/21.
//

#ifndef SHARP_IDLE_SCHEDULER_H
#define SHARP_IDLE_SCHEDULER_H

#include "scheduler.h"

#define MAX_TASK_RELEASE 3000                /* Max amount of tasks to release in one load */
#define IDLE_SCHED_CLOCK_CYCLE (CLOCK_CYCLE * 3) /* Max time given to the idle scheduler per second in microseconds */
#define MIN_DELAY_TIME (SCHEDULER_PERIOD / TIME_UNIT_MULT) /* The minimum amount of time to consider a task to be "idle" */

extern sched_task *idle_tasks, *last_idle_task;
extern Thread *idleSched;
extern recursive_mutex idle_mutex;

#ifdef WIN32_
DWORD WINAPI
#endif
#ifdef POSIX_
void*
#endif
idle_sched_main(void *);

void create_idle_scheduler();
void start_idle_scheduler();
void run_idle_scheduler();
void release_tasks();
void post_idle_task(sched_task *);
bool is_idle_task(sched_task *);
bool can_release_idle_task(sched_task *);
void release_idle_task(sched_task *);
fiber* locate_idle_task(uInt);
void queue_idle_task(sched_task* task);
void add_idle_tasks();

#endif //SHARP_IDLE_SCHEDULER_H
