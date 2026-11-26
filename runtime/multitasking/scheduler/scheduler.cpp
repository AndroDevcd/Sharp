//
// Created by bknun on 9/19/2022.
//
// See scheduler.h for the overview of how the scheduler works.
//

#include "scheduler.h"
#include "../../util/time.h"
#include "../../memory/garbage_collector.h"
#include "../thread_state.h"
#include "../thread/sharp_thread.h"
#include "../../virtual_machine.h"
#include "../thread/thread_controller.h"
#include "../fiber/fiber.h"
#include "idle_scheduler.h"
#include "../../memory/tracing_collector.h"
#include "../fiber/task_controller.h"
#include "../../telemetry/telemetry.h"
#include "../../../debugger/hooks.h"        // what the debugger build (sdb) adds; empty in the plain VM
#include <unordered_map>
#include <vector>
#include <queue>
#include <chrono>
#include <condition_variable>
#ifdef __linux__
#include <sys/prctl.h>
#endif

uInt schedTime = 0, clocks = 0;
uInt taskCount = 0; /* Read-Only */

recursive_mutex task_mutex;
recursive_mutex thread_mutex;
recursive_mutex unsched_mutex;

_sched_thread *sched_threads = nullptr;
_unsched_thread *unsched_threads = nullptr;

// ---------------------------------------------------------------------------------
// Scheduler state. Everything in this block is guarded by task_mutex.
// ---------------------------------------------------------------------------------

/** every live fiber, by id */
static std::unordered_map<uInt, fiber*> tasks;

/** ready unbound fibers (FIFO, linked through fiber->readyNext) */
static fiber *unboundHead = nullptr, *unboundTail = nullptr;

/** threads sleeping in sched_next_task() because they had nothing to run */
static std::vector<sharp_thread*> parkedThreads;

/** fibers waiting for an object lock, by id (ids, not pointers: a waiter may be killed and freed) */
static std::vector<uInt> lockWaiters;

/** sleeping fibers ordered by wake time. Entries are never removed early: a stale
 *  entry (the fiber was woken, killed or re-delayed since) is ignored when it comes due. */
struct timer_entry {
    uInt time;
    uInt id;
    bool operator>(const timer_entry &other) const { return time > other.time; }
};
static std::priority_queue<timer_entry, std::vector<timer_entry>, std::greater<timer_entry>> timers;

// ---------------------------------------------------------------------------------
// Monitor wake-up (its own small lock so nobody waits on task_mutex to sleep)
// ---------------------------------------------------------------------------------
static std::mutex monitor_mutex;
static std::condition_variable monitor_cv;
static bool monitorKick = false;

/** Make the monitor re-evaluate now instead of at the end of its current sleep */
static void kick_monitor() {
    {
        std::lock_guard<std::mutex> lk(monitor_mutex);
        monitorKick = true;
    }
    monitor_cv.notify_one();
}

void __usleep(unsigned int usec)
{
    // a sleeping thread touches no objects: the collector need not wait for it
    gc_safe_region asleep;
#ifdef COROUTINE_DEBUGGING
    if(thread_self)
        thread_self->timeSleeping += usec;
#endif

    std::this_thread::sleep_for(std::chrono::microseconds(usec));
}

uInt sched_now_micros() {
    // (SDB_SCHEDULER_TIME: the debugger leaves out the time the program was held; the plain VM changes nothing)
    return SDB_SCHEDULER_TIME((uInt) std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());
}

// ---------------------------------------------------------------------------------
// Ready queues (task_mutex must be held)
// ---------------------------------------------------------------------------------

static void queue_push(fiber *&head, fiber *&tail, fiber *fib) {
    fib->readyNext = nullptr;
    if(tail) tail->readyNext = fib;
    else head = fib;
    tail = fib;
    fib->queued = true;
}

static fiber* queue_pop(fiber *&head, fiber *&tail) {
    fiber *fib = head;
    if(fib) {
        head = fib->readyNext;
        if(head == nullptr) tail = nullptr;
        fib->readyNext = nullptr;
        fib->queued = false;
    }
    return fib;
}

static void unpark(sharp_thread *thread) {
    if(thread->parked) {
        thread->parked = false;
        for(size_t i = 0; i < parkedThreads.size(); i++) {
            if(parkedThreads[i] == thread) {
                parkedThreads[i] = parkedThreads.back();
                parkedThreads.pop_back();
                break;
            }
        }
    }
}

/**
 * Give `fib` to a thread that is asleep waiting for work and wake it.
 * The fiber is marked as attached right away so nobody else picks it up.
 */
static void hand_off(sharp_thread *thread, fiber *fib) {
    unpark(thread);
    fib->attachedThread = thread;
    thread->queue = fib;
    send_notification(&thread->queueNotification);
}

/**
 * Put a fiber that can run right now where a thread will find it.
 *
 * allowHandOff = false is used by a thread that is about to pick its next fiber
 * itself: queueing instead of waking another thread lets it simply continue
 * with the same fiber when nothing else is waiting (a yield with no other work
 * must not cost a thread wake-up).
 */
static void enqueue_ready(fiber *fib, bool allowHandOff) {
    if(fib->queued) return;

    sharp_thread *bound = fib->boundThread;
    if(bound != nullptr) {
        if(allowHandOff && bound->parked && bound->queue == nullptr)
            hand_off(bound, fib);
        else
            queue_push(bound->readyHead, bound->readyTail, fib);
    } else {
        if(allowHandOff && !parkedThreads.empty())
            hand_off(parkedThreads.back(), fib);
        else
            queue_push(unboundHead, unboundTail, fib);
    }
}

/**
 * Decide where a fiber that is not running waits: nowhere (suspended by
 * fiber.suspend), the timer heap, the lock wait list, or a ready queue.
 */
static void file_fiber(fiber *fib, bool allowHandOff) {
    if(!fib->wakeable)
        return; // parked until unsuspend_task() wakes it

    if(fib->delayTime != -1) {
        if((long long)sched_now_micros() < fib->delayTime) {
            if(fib->timerArmed != fib->delayTime) {
                fib->timerArmed = fib->delayTime;
                bool earliest = timers.empty() || (uInt)fib->delayTime < timers.top().time;
                timers.push({ (uInt)fib->delayTime, fib->id });
                if(earliest) kick_monitor(); // the monitor is sleeping until a later time
            }
            return;
        }

        fib->delayTime = -1;
    }

    if(fib->f_lock != nullptr && fib->f_lock->id != -1) {
        if(!fib->lockWaiting) {
            fib->lockWaiting = true;
            lockWaiters.push_back(fib->id);
        }
        return;
    }

    enqueue_ready(fib, allowHandOff);
}

void sched_wake(fiber *fib) {
    guard_mutex(task_mutex)

    // A fiber that a thread is still running is filed by that thread when it
    // lets go of it (sched_next_task), using the state we see here.
    if(fib->attachedThread != nullptr || fib->queued)
        return;
    if(fib->state != FIB_SUSPENDED && fib->state != FIB_CREATED)
        return;

    file_fiber(fib, true);
}

void sched_lock_released(fib_mutex *mut) {
    guard_mutex(task_mutex)
    if(lockWaiters.empty()) return;

    std::vector<uInt> waiting;
    waiting.swap(lockWaiters);

    for(uInt id : waiting) {
        auto it = tasks.find(id);
        if(it == tasks.end()) continue; // killed while it waited

        fiber *fib = it->second;
        if(fib->f_lock == nullptr || fib->f_lock->id == -1) {
            // the lock is free: let it retry (it re-executes its LOCK instruction)
            fib->lockWaiting = false;
            sched_wake(fib);
        } else {
            lockWaiters.push_back(id);
        }
    }
}

void sched_register(fiber *fib, bool makeReady) {
    guard_mutex(task_mutex)
    tasks[fib->id] = fib;
    taskCount = tasks.size();

    if(makeReady)
        sched_wake(fib);
}

void sched_visit_fibers(void (*visit)(fiber*, void*), void *arg) {
    guard_mutex(task_mutex)
    for(auto &entry : tasks)
        visit(entry.second, arg);
}

fiber *locate_task(uInt taskId) {
    guard_mutex(task_mutex)
    auto it = tasks.find(taskId);
    return it != tasks.end() ? it->second : nullptr;
}

void sched_reap(fiber *fib) {
    guard_mutex(task_mutex)

    // Only a dead fiber that nothing refers to may be freed. A fiber that is
    // still on a ready queue is freed by whoever pops it.
    if(fib->state != FIB_KILLED || fib->attachedThread != nullptr || fib->queued)
        return;

    tasks.erase(fib->id);
    taskCount = tasks.size();

    fib->free(); // also releases every object lock it held
    std::free(fib);

    sched_lock_released(nullptr);
}

void kill_bound_tasks(sharp_thread* thread) {
    guard_mutex(task_mutex)

    std::vector<fiber*> bound;
    for(auto &entry : tasks) {
        if (is_bound(entry.second, thread))
            bound.push_back(entry.second);
    }

    for(fiber *fib : bound) {
        set_task_state(NULL, fib, FIB_KILLED, NO_DELAY);
        sched_reap(fib);
    }
}

// ---------------------------------------------------------------------------------
// Workers
// ---------------------------------------------------------------------------------

/**
 * The thread stops running its current fiber. Frees it if it is dead, otherwise
 * files it wherever it has to wait.
 */
static void detach_current(sharp_thread *thread) {
    fiber *fib = thread->task;
    thread->task = nullptr;
    fib->attachedThread = nullptr;

    if(fib->finished || fib->state == FIB_KILLED) {
        set_task_state(thread, fib, FIB_KILLED, NO_DELAY);
        sched_reap(fib);
        return;
    }

    // Still RUNNING means it was preempted (time slice) or lost a race for a
    // lock. delay()/suspend already moved it to SUSPENDED themselves.
    if(fib->state == FIB_RUNNING) {
        fib->state = FIB_SUSPENDED;
        fib->delayTime = -1;
    }

    file_fiber(fib, false);
}

static void attach(sharp_thread *thread, fiber *fib) {
    fib->attachedThread = thread;
    thread->task = fib;
    thread->state = THREAD_RUNNING;
    enable_context_switch(thread, false);
    set_task_state(thread, fib, FIB_RUNNING, NO_DELAY);
}

/**
 * Take the next fiber this thread may run off the ready queues, or nullptr.
 * Fibers whose situation changed while they were queued are dealt with here.
 */
static fiber* pop_next(sharp_thread *thread) {
    for(;;) {
        fiber *fib = nullptr;

        // bound fibers first, but let an unbound one through regularly so a
        // thread with a busy queue of its own does not starve the shared queue
        bool unboundFirst = unboundHead != nullptr
                && (++thread->pickCount % SCHED_UNBOUND_PICK_INTERVAL) == 0;

        if(!unboundFirst) fib = queue_pop(thread->readyHead, thread->readyTail);
        if(fib == nullptr) fib = queue_pop(unboundHead, unboundTail);
        if(fib == nullptr && unboundFirst) fib = queue_pop(thread->readyHead, thread->readyTail);
        if(fib == nullptr) return nullptr;

        if(fib->state == FIB_KILLED) {
            sched_reap(fib); // killed while it was queued
            continue;
        }

        if(fib->attachedThread != nullptr)
            continue; // should not happen, never run a fiber twice

        if(!fib->wakeable)
            continue; // suspended while it was queued, unsuspend queues it again

        if(fib->boundThread != nullptr && fib->boundThread != thread) {
            enqueue_ready(fib, true); // was bound to another thread while it was queued
            continue;
        }

        return fib;
    }
}

/**
 * After a thread took a fiber for itself: if more unbound fibers are waiting
 * and other threads are asleep, put them to work.
 */
static void spill_unbound() {
    while(unboundHead != nullptr && !parkedThreads.empty()) {
        fiber *fib = queue_pop(unboundHead, unboundTail);

        if(fib->state == FIB_KILLED) sched_reap(fib);
        else if(fib->wakeable && fib->attachedThread == nullptr) enqueue_ready(fib, true);
    }
}

bool sched_next_task(sharp_thread *thread) {
    TELEMETRY_COUNT(tm_context_switches)

    for(;;) {
        {
            guard_mutex(task_mutex)
            unpark(thread);

            if(thread->task != nullptr)
                detach_current(thread);

            // a fiber that was handed to us while we slept, otherwise the queues
            fiber *fib = thread->queue;
            if(fib != nullptr) {
                thread->queue = nullptr;

                if(fib->state == FIB_KILLED || !fib->wakeable) {
                    fib->attachedThread = nullptr;
                    sched_reap(fib); // no-op unless it is dead
                    fib = nullptr;
                }
            }

            if(fib == nullptr)
                fib = pop_next(thread);

            if(fib != nullptr) {
                attach(thread, fib);
                spill_unbound();
                return true;
            }

            if(thread->state == THREAD_KILLED || hasSignal(thread->signal, tsig_kill))
                return false;

            // nothing to run: sleep until a fiber is handed to us (or we are
            // told to suspend/die, see queue_filled())
            thread->parked = true;
            parkedThreads.push_back(thread);
            thread->state = THREAD_SCHED;
        }

        {
            TELEMETRY_SCOPE(tm_mode_sched_wait)
            TELEMETRY(telemetry_switch_timer parkTimer;)
            wait_for_notification(&thread->queueNotification);
        }

        if(hasSignal(thread->signal, tsig_suspend))
            suspend_self();
    }
}

void sched_thread_exit(sharp_thread *thread) {
    guard_mutex(task_mutex)
    unpark(thread);

    // Work with ids from here on: killing the bound fibers below may free the
    // very fibers these pointers refer to.
    Int handedId = -1, currentId = -1;

    fiber *handed = thread->queue;
    if(handed != nullptr) {
        thread->queue = nullptr;
        handed->attachedThread = nullptr;
        handedId = handed->id;
    }

    fiber *current = thread->task;
    if(current != nullptr) {
        thread->task = nullptr;
        current->attachedThread = nullptr;
        currentId = current->id;
        set_task_state(NULL, current, FIB_KILLED, NO_DELAY);
    }

    // nobody will ever pop this thread's queue again
    while(queue_pop(thread->readyHead, thread->readyTail) != nullptr) { }

    kill_bound_tasks(thread);

    if(currentId != -1) {
        fiber *fib = locate_task(currentId);
        if(fib) sched_reap(fib);
    }

    if(handedId != -1) {
        fiber *fib = locate_task(handedId);
        if(fib) sched_wake(fib); // an unbound fiber we never got to run: back to the shared queue
    }
}

// ---------------------------------------------------------------------------------
// Threads known to the scheduler
// ---------------------------------------------------------------------------------

void sched_unsched_threads() {
    // Lock order: thread_mutex first, then unsched_mutex - the order
    // get_thread() (thread_controller.cpp) uses. This function used to take
    // unsched_mutex and then, inside queue_thread(), thread_mutex: with a Sharp
    // thread in get_thread() at the same moment each side held the mutex the
    // other one waited for, and the VM hung (about 1 run in 40 of a program
    // that starts many threads). Both mutexes are recursive, so queue_thread()
    // locking thread_mutex again is fine.
    std::lock_guard<recursive_mutex> threadListGuard(thread_mutex);
    guard_mutex(unsched_mutex)

    while (unsched_threads != nullptr) {
        if (!queue_thread(unsched_threads->thread)) {
            return;
        }

        auto *tmp = unsched_threads->next;
        std::free(unsched_threads);
        unsched_threads = tmp;
    }
}

void sched_unsched_items() {
    if(unsched_threads) sched_unsched_threads();
}

void post(sharp_thread* thread) {
    guard_mutex(unsched_mutex)
    _unsched_thread *t = (_unsched_thread*)malloc(sizeof(_unsched_thread));

    if(t)
    {
        t->thread = thread;
        t->next = unsched_threads;
        unsched_threads = t;
    }
}

_sched_thread* get_sched_threads() {
    return sched_threads;
}

bool queue_thread(sharp_thread* thread) {
    guard_mutex(thread_mutex)
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

void dispose(_sched_thread *scht) {
    guard_mutex(thread_mutex)
    sharp_thread *thread = scht->thread;

    if (thread->state == THREAD_KILLED || thread->terminated)
    {
        if(scht == sched_threads)
            sched_threads = scht->next;

        if(scht->prev != NULL)
            scht->prev->next = scht->next;
        if(scht->next != NULL)
            scht->next->prev = scht->prev;

        free_struct(thread);
        std::free (thread);
        std::free (scht);
    }
}

// ---------------------------------------------------------------------------------
// The monitor
// ---------------------------------------------------------------------------------

/** Move every sleeping fiber whose time has come to a ready queue (task_mutex held) */
static void fire_timers(uInt now) {
    while(!timers.empty() && timers.top().time <= now) {
        timer_entry entry = timers.top();
        timers.pop();

        auto it = tasks.find(entry.id);
        if(it == tasks.end()) continue; // the fiber is gone

        fiber *fib = it->second;
        if(fib->delayTime != (long long)entry.time) continue; // stale: woken or re-delayed since

        fib->delayTime = -1;
        fib->timerArmed = -1;
        sched_wake(fib);
    }
}

static uInt time_slice_of(sharp_thread *thread) {
    switch(thread->priority) {
        case THREAD_PRIORITY_LOW: return LPTSI;
        case THREAD_PRIORITY_HIGH: return HPTSI;
        default: return NPTSI;
    }
}

void run_scheduler() {
    TELEMETRY(telemetry_name_thread("scheduler monitor (os main thread)");)

#ifdef __linux__
    // Linux rounds the wake-up time of a sleeping thread by its "timer slack"
    // (50us by default). This thread's sleeps decide how punctual delay() is,
    // so ask for 1us instead.
    prctl(PR_SET_TIMERSLACK, 1UL, 0UL, 0UL, 0UL);
#endif

    std::vector<sharp_thread*> running;

    do {
        clocks++;

        if(vm.state >= VM_SHUTTING_DOWN) {
            while(vm.state != VM_TERMINATED) {
                __usleep(1000);
            }
#ifdef COROUTINE_DEBUGGING
            cout << "total sched clocks: " << clocks << endl;
#endif
            return;
        }

        sched_unsched_items();

        uInt now = sched_now_micros();
        uInt wakeAt = now + SCHED_TICK_MICROS;
        bool unboundWaiting;
        schedTime = now;

        // ---- timers
        {
            guard_mutex(task_mutex)
            fire_timers(now);

            if(!timers.empty() && timers.top().time < wakeAt)
                wakeAt = timers.top().time;
            unboundWaiting = unboundHead != nullptr;
        }

        // ---- threads: free the dead ones, note the ones that are running a fiber.
        // (only this thread ever frees a sharp_thread, so the pointers collected
        //  here stay valid after thread_mutex is released)
        running.clear();
        {
            guard_mutex(thread_mutex)
            _sched_thread *scht = sched_threads;
            while(scht != nullptr) {
                auto next = scht->next;

                if(can_dispose(scht)) dispose(scht);
                else if(scht->thread->state == THREAD_RUNNING) running.push_back(scht->thread);
                scht = next;
            }
        }

        // ---- time slices: a thread that has run the same fiber for longer than
        // its slice is asked to switch, but only when another fiber it could run
        // is actually waiting.
        if(!running.empty()) {
            guard_mutex(task_mutex)

            for(sharp_thread *thread : running) {
                if(thread->task == nullptr || thread->state != THREAD_RUNNING)
                    continue; // gc/idle threads, or it parked in the meantime

                // `now` was read at the top of this round. A fiber that started
                // AFTER that has lastRanMicros > now, and both are unsigned:
                // now - lastRanMicros would wrap around to an enormous number
                // and the fiber would be switched out the moment it began.
                // (It was: about one coroutine in a thousand lost its turn
                //  right after starting - "first come first served" on one
                //  thread came out as b, c, d, a.)
                if(now <= thread->lastRanMicros)
                    continue;   // it has run for no time at all yet

                // (SDB_KEEPS_FIBER: a fiber the debugger is stepping stays on its thread; always false in the plain VM)
                if((thread->readyHead != nullptr || unboundWaiting)
                    && (now - thread->lastRanMicros) > time_slice_of(thread)
                    && !SDB_KEEPS_FIBER(thread)) {
                    enable_context_switch(thread, true);
                }
            }
        }

        // ---- sleep until the next timer is due, a tick passes, or an earlier
        // timer is added (kick_monitor)
        {
            std::unique_lock<std::mutex> lk(monitor_mutex);
            if(!monitorKick) {
                uInt current = sched_now_micros();
                if(wakeAt > current)
                    monitor_cv.wait_for(lk, std::chrono::microseconds(wakeAt - current));
            }
            monitorKick = false;
        }
    } while(true);
}
