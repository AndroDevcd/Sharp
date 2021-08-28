//
// Created by BNunnally on 9/5/2020.
//

#ifndef SHARP_SCHEDULER_H
#define SHARP_SCHEDULER_H

#include "../../../stdimports.h"
#include "../List.h"
#include <mutex>

#define TIME_UNIT_MULT 1000          /* Represents how many microseconds/millisecond */
#define LPTSI (25 * TIME_UNIT_MULT)  /* low priority time slice interval 25ms */  // ~40 max fibers/second
#define NPTSI (50 * TIME_UNIT_MULT)  /* norm priority time slice interval 50ms */ // ~20 max fibers/second
#define HPTSI (85 * TIME_UNIT_MULT)  /* high priority time slice interval 85ms */ // ~11 max fibers/second
#define CLOCK_CYCLE 500              /* Time between clock cycle switches 500us */
#define POST_IDLE_FREQUENCY ((TIME_UNIT_MULT / CLOCK_CYCLE) * 10)

// A task will be skipped 3 times while its in "locking" state to prevent deadlocks
#define LOCK_LIMIT 3

class fiber;
struct sched_task {
    fiber *task;
    sched_task *next, *prev;
};

class Thread;
struct _sched_thread {
    Thread *thread;
    _sched_thread *next, *prev;
};

extern recursive_mutex task_mutex;
extern recursive_mutex thread_mutex;
extern uInt clocks;
extern sched_task *sched_tasks;
extern _sched_thread *sched_threads;
extern _sched_thread *last_thread;
extern _List<Thread*> unSchedThreads;
extern _List<fiber*> unSchedTasks;
extern uInt schedTime;

void run_scheduler();
bool is_thread_ready(Thread *thread);
void __usleep(unsigned int usec);
void sched(Thread * thread);
bool can_sched(fiber*, Thread*);
bool can_dispose(sched_task*);
void clear_tasks();
void dispose(sched_task*);
void post(Thread*);
void post(fiber*);
void sched_unsched_items();
bool queue_task(fiber*);
bool queue_thread(Thread*);
void post_idle_tasks();
void shift_to_next_task();


#endif //SHARP_SCHEDULER_H
