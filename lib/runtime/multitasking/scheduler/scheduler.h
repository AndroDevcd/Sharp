//
// Created by bknun on 9/19/2022.
//

#ifndef SHARP_SCHEDULER_H
#define SHARP_SCHEDULER_H

#include "../../../../stdimports.h"

#define TIME_UNIT_MULT 1000          /* Represents how many microseconds/millisecond */
#define LPTSI ((TIME_UNIT_MULT * 1000) / 80)  /* low priority time slice interval 80 ctx switches/sec */  // ~12.5ms
#define NPTSI ((TIME_UNIT_MULT * 1000) / 150)  /* norm priority time slice interval 150 ctx switches/sec */ // ~6.6ms
#define HPTSI ((TIME_UNIT_MULT * 1000) / 250)  /* high priority time slice interval 250 ctx switches/sec */ // ~4ms

struct fiber;
struct sched_task {
    fiber *task;
    sched_task *next, *prev;
};

struct sharp_thread;
struct _sched_thread {
    sharp_thread *thread;
    _sched_thread *next, *prev;
};

struct unsched_task {
    fiber *task;
    unsched_task *next;
};

struct _unsched_thread {
    sharp_thread *thread;
    _unsched_thread *next;
};

extern recursive_mutex task_mutex;
extern recursive_mutex thread_mutex;
extern uInt clocks;
extern unsched_task *unsched_tasks;
extern _unsched_thread *unsched_threads;
extern uInt schedTime;
extern uInt taskCount;

void run_scheduler();
bool is_thread_ready(sharp_thread *thread);
void __usleep(unsigned int usec);
bool can_dispose(sched_task*);
void clear_tasks();
void dispose(sched_task*);
void remove_task(sched_task *task);
void post(sharp_thread*);
fiber *locate_task(uInt taskId);
void dispose(_sched_thread*);
void kill_bound_tasks(sharp_thread* thread);
bool is_runnable(fiber *task, sharp_thread *thread, bool execUnbound);
bool can_purge(fiber *task);
void post(fiber*);
void sched_unsched_items();
bool queue_task(fiber*);
bool queue_thread(sharp_thread*);
_sched_thread* get_sched_threads();

#endif //SHARP_SCHEDULER_H
