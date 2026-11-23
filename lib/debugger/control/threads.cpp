//
// See threads.h.
//

#include "threads.h"
#include "control.h"
#include "../../runtime/multitasking/thread/sharp_thread.h"
#include "../../runtime/multitasking/fiber/fiber.h"
#include "../../runtime/multitasking/scheduler/scheduler.h"
#include "../../core/thread_state.h"
#include <algorithm>
#include <chrono>

/** where a fiber is, in words (fiber_row.place); "" when it has not started */
static string place_of(fiber *task) {
    std::vector<code_place> calls = call_stack_of(task);
    if(calls.empty()) return "";

    for(code_place &call : calls) {
        if(!is_library_function(call.function)) return describe_place(call);
    }
    return describe_place(calls.front());
}

/** every call that led to where a fiber is, in one line */
static string calls_of(fiber *task) {
    string all;
    for(code_place &call : call_stack_of(task)) all += describe_place(call) + " ";
    return all;
}

// ---------------------------------------------------------------------------------
// Threads
// ---------------------------------------------------------------------------------

static string state_of(sharp_thread *thread, bool held) {
    if(thread->id == gc_threadid || thread->id == idle_threadid) return "VM";
    if(thread->state == THREAD_KILLED || thread->exited || thread->terminated) return "finished";
    if(thread->state == THREAD_CREATED) return "not started";
    if(held) return "running";
    if(thread->state == THREAD_SUSPENDED || thread->suspended) return "suspended";
    if(thread->parked || thread->state == THREAD_SCHED || thread->task == nullptr) return "waiting for a fiber";
    if(__atomic_load_n(&thread->gcSafe, __ATOMIC_SEQ_CST) != 0) return "extern c++";
    return "extern c++ (busy)";
}

std::vector<thread_row> threads_now() {
    std::vector<thread_row> rows;
    {
        for(sharp_thread *thread : sdb_all_threads()) {
            thread_row row;
            row.id = thread->id;
            row.name = thread->name;
            row.own = thread->id == gc_threadid || thread->id == idle_threadid;
            row.state = state_of(thread, sdb_is_held(thread));
            row.task = row.own ? nullptr : thread->task;
            if(row.task != nullptr) {
                row.fiberId = row.task->id;
                row.place = place_of(row.task);
            }
            rows.push_back(row);
        }
    }
    std::sort(rows.begin(), rows.end(), [](const thread_row &a, const thread_row &b) { return a.id < b.id; });
    return rows;
}

// ---------------------------------------------------------------------------------
// Fibers
// ---------------------------------------------------------------------------------

static string state_of(fiber *task) {
    stringstream words;

    if(task->state == FIB_KILLED || task->finished) return "finished";
    if(task->attachedThread != nullptr) {
        words << "running on thread " << task->attachedThread->id;
        return words.str();
    }
    if(task->state == FIB_CREATED) return "not started";
    if(!task->wakeable) return "suspended";

    if(task->delayTime != -1) {
        // the scheduler's own clock: it stands still while the program is held
        long long left = task->delayTime - (long long)sched_now_micros();
        if(left < 0) left = 0;
        words << "sleeping, " << (left + 500) / 1000 << " ms left";
        return words.str();
    }
    if(task->f_lock != nullptr && task->f_lock->id != -1) {
        words << "waiting for a lock that fiber " << task->f_lock->id << " holds";
        return words.str();
    }
    if(task->queued) return "ready";
    return "waiting";
}

static void add_fiber_row(fiber *task, void *into) {
    fiber_row row;
    row.id = task->id;
    row.name = task->name;
    row.state = state_of(task);
    if(task->attachedThread != nullptr) row.threadId = task->attachedThread->id;
    if(task->boundThread != nullptr) row.boundTo = task->boundThread->id;
    row.place = place_of(task);
    row.calls = calls_of(task);
    row.task = task;
    ((std::vector<fiber_row>*)into)->push_back(row);
}

std::vector<fiber_row> fibers_now() {
    std::vector<fiber_row> rows;
    sched_visit_fibers(add_fiber_row, &rows);       // (holds the scheduler's lock during the calls)
    std::sort(rows.begin(), rows.end(), [](const fiber_row &a, const fiber_row &b) { return a.id < b.id; });
    return rows;
}
