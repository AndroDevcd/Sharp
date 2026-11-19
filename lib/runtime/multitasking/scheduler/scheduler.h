//
// Created by bknun on 9/19/2022.
//
// Sharp fiber scheduler
// =====================
//
// HOW IT WORKS (read this first)
//
// A *fiber* is a Sharp coroutine: it owns its VM stack, registers and pc, so
// switching between fibers is just pointing a thread at a different fiber.
// A *thread* (sharp_thread) is an OS thread that runs fibers, one at a time.
// A fiber is either BOUND to one thread (only that thread may run it) or UNBOUND
// (any thread may run it).
//
// The scheduler is event driven. Nothing is polled:
//
//   * READY QUEUES hold only fibers that can run right now.
//       - every thread has its own queue for the fibers bound to it
//       - one shared queue holds the unbound fibers
//
//   * WORKERS SWITCH FIBERS THEMSELVES. When the fiber a thread is running
//     yields, sleeps, blocks or finishes, the thread calls sched_next_task():
//     it puts the old fiber where it belongs (see below) and takes the next
//     ready one. No other thread is involved, so a switch is a few queue
//     operations. A thread with nothing to run PARKS (sleeps on its condition
//     variable) and costs no cpu.
//
//   * A fiber that is not ready waits in exactly one place, and the event that
//     ends the wait puts it back on a ready queue:
//
//         waiting for...            lives in...          woken by...
//         ------------------------  -------------------  ---------------------------
//         delay(ms)                 the timer heap       the monitor thread, when due
//         an object lock (LOCK)     the lock wait list   unlock_object()
//         unsuspend (fiber.suspend) nowhere (registry)   unsuspend_task()
//
//     When a fiber becomes ready and a suitable thread is parked, the fiber is
//     handed straight to that thread (thread->queue) and the thread is woken.
//
//   * THE MONITOR (run_scheduler, on the OS main thread) is the only background
//     work. It sleeps until the next timer is due or SCHED_TICK_MICROS passes,
//     then: moves expired timers to the ready queues, asks threads that have
//     used up their time slice to switch (only if something else is waiting to
//     run), and frees threads that have exited.
//
// LOCKING
//   task_mutex guards ALL fiber scheduling state: the registry, every ready
//   queue, the timer heap, the lock wait list, the parked thread list and these
//   fiber fields: state, attachedThread, boundThread, wakeable, delayTime,
//   queued, readyNext. It is recursive. Rules:
//     - never take thread_mutex while holding task_mutex
//     - thread->mut (the signal bits) may be taken while holding task_mutex,
//       never the other way round
//
// TIME
//   Every scheduler time (fiber->delayTime, thread->lastRanMicros) comes from
//   sched_now_micros(), a monotonic clock. It is not wall clock time.
//

#ifndef SHARP_SCHEDULER_H
#define SHARP_SCHEDULER_H

#include "../../../../stdimports.h"

#define TIME_UNIT_MULT 1000          /* Represents how many microseconds/millisecond */
#define LPTSI ((TIME_UNIT_MULT * 1000) / 80)  /* low priority time slice interval 80 ctx switches/sec */  // ~12.5ms
#define NPTSI ((TIME_UNIT_MULT * 1000) / 150)  /* norm priority time slice interval 150 ctx switches/sec */ // ~6.6ms
#define HPTSI ((TIME_UNIT_MULT * 1000) / 250)  /* high priority time slice interval 250 ctx switches/sec */ // ~4ms

/* longest the monitor sleeps when no timer is due sooner (time slice checks, thread cleanup) */
#define SCHED_TICK_MICROS 1000
/* a thread takes an unbound fiber ahead of its own bound fibers once every this many picks */
#define SCHED_UNBOUND_PICK_INTERVAL 4

struct fiber;
struct fib_mutex;
struct sharp_thread;

struct _sched_thread {
    sharp_thread *thread;
    _sched_thread *next, *prev;
};

struct _unsched_thread {
    sharp_thread *thread;
    _unsched_thread *next;
};

extern recursive_mutex task_mutex;      // the scheduler lock, see LOCKING above
extern recursive_mutex thread_mutex;    // guards the list of threads (sched_threads)
extern recursive_mutex unsched_mutex;   // guards threads that were created but not yet listed
extern uInt clocks;
extern _unsched_thread *unsched_threads;
extern uInt schedTime;
extern uInt taskCount;                  // number of live fibers (read only outside the scheduler)

// ---- monitor ----------------------------------------------------------------------
void run_scheduler();
void __usleep(unsigned int usec);
uInt sched_now_micros();                // monotonic microseconds

// ---- threads ----------------------------------------------------------------------
void post(sharp_thread*);               // announce a new thread to the scheduler
void dispose(_sched_thread*);
void sched_unsched_items();
bool queue_thread(sharp_thread*);
_sched_thread* get_sched_threads();

// ---- fibers -----------------------------------------------------------------------
/** Add a new fiber to the registry. makeReady = false when the creator runs it itself (a thread's first fiber). */
void sched_register(fiber *fib, bool makeReady);
/** Find a live fiber by id. Hold task_mutex for as long as you use the result. */
fiber *locate_task(uInt taskId);

/**
 * Call visit(fiber, arg) for every fiber that exists (ready, running, waiting,
 * not started). Used by the garbage collector to find what fibers refer to.
 * task_mutex is held during the calls.
 */
void sched_visit_fibers(void (*visit)(fiber*, void*), void *arg);
/** Something changed that may let `fib` run (unsuspended, timer due, lock released): queue it if it can run now. */
void sched_wake(fiber *fib);
/** Free a KILLED fiber that no thread is running. Safe to call in any state, does nothing if it is not time yet. */
void sched_reap(fiber *fib);
/** An object lock was released: wake the fibers that were waiting for it (nullptr = re-check every waiter). */
void sched_lock_released(fib_mutex *mut);
void kill_bound_tasks(sharp_thread* thread);

// ---- workers ----------------------------------------------------------------------
/**
 * The context switch. Called by a thread whose current fiber stopped running
 * (or that has none): files the old fiber away and attaches the next ready one,
 * sleeping until there is one. Returns false if the thread was told to die
 * while it waited (thread->task is nullptr then).
 */
bool sched_next_task(sharp_thread *thread);
/** A thread is exiting: give back anything handed to it, kill and free the fibers bound to it. */
void sched_thread_exit(sharp_thread *thread);

#endif //SHARP_SCHEDULER_H
