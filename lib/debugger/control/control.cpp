//
// See control.h.
//

#include "control.h"
#include "../hooks.h"
#include "../../runtime/virtual_machine.h"
#include "../../runtime/multitasking/thread/sharp_thread.h"
#include "../../runtime/multitasking/fiber/fiber.h"
#include "../../runtime/memory/tracing_collector.h"
#include "../../runtime/multitasking/scheduler/scheduler.h"
#include "../../core/thread_state.h"
#include <condition_variable>
#include <unordered_map>
#include <chrono>
#include <thread>
#include <algorithm>
#include <csignal>
#include "../../runtime/multitasking/thread/thread_controller.h"

volatile int sdb_attention = 0;

// ---------------------------------------------------------------------------------
// What both sides share. control_mut guards all of it; the things a running
// thread reads before EVERY instruction (`holding`, `armed`, `stepping`) can
// also be read without it.
// ---------------------------------------------------------------------------------

// (made with `new` and never freed: the VM ends the process with exit() from any thread, and exit()
//  would wait forever freeing a condition another thread still waits on)
static std::mutex &control_mut = *new std::mutex();
static std::condition_variable &control_changed = *new std::condition_variable();     // a thread stopped, the program was let go, or it ended

static std::atomic<bool> holding(false);            // threads that reach an instruction wait
static std::atomic<bool> pause_wanted(false);       // Ctrl+C was pressed and no stop has answered it yet
static stop_report current_stop;                    // filled by the first thread that stops
static std::vector<stop_report> later_stops;        // threads that stopped for a reason of their own meanwhile: reported next
static std::vector<sharp_thread*> held_threads;     // the threads waiting in the hook
static const std::function<void()> *work_wanted = nullptr;      // sdb_run_on_held_thread: for the first held thread that sees it
static bool work_done = false;
static bool ended = false;
static int exit_code = 0;

static std::vector<breakpoint> breakpoints;
static int last_number = 0;
static std::vector<code_place> once_places;         // sdb_stop_once_at

/**
 * Where the program stops, for the hook: the address of the instruction -> a
 * breakpoint's number, or ONCE for a place of sdb_stop_once_at. A running
 * thread looks in it without a lock, so it is never changed: every change
 * makes a new one and swaps the pointer. (The old ones are kept; a thread may
 * still be reading one. They are a few bytes each and there are as many as a
 * person typed `break` and `clear`.)
 */
#define ONCE (-1)
typedef std::unordered_map<const opcode_instr*, int> armed_places;
static std::atomic<const armed_places*> armed(nullptr);
static std::vector<const armed_places*> old_armed;

/**
 * The step under way. `stepping` is the fiber it is about (null: none); the
 * rest is where that fiber was when it was let go, and is written only while
 * every thread is held.
 */
static std::atomic<fiber*> stepping(nullptr);
static step_kind step_wanted = STEP_NONE;
static sharp_function *step_function = nullptr;
static Int step_depth = 0;                          // how many calls deep the fiber was
static Int step_line = -1;

// ---------------------------------------------------------------------------------
// The scheduler's time (hooks.h, SDB_SCHEDULER_TIME): real time less all the
// time the program was held. A lock of its own: the scheduler asks from its
// monitor and from every thread that switches fibers.
// ---------------------------------------------------------------------------------

static std::mutex &time_mut = *new std::mutex();
static uint64_t held_since = 0;                     // real time the program has been held since (0: it is not held)
static uint64_t held_total = 0;                     // all the time it was held before

static uint64_t real_micros() {
    return (uint64_t) std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
}

uint64_t sdb_scheduler_time(uint64_t real) {
    std::lock_guard<std::mutex> lock(time_mut);
    return (held_since != 0 ? held_since : real) - held_total;
}

/** the program is held from now / is let go now */
static void stop_the_clock() {
    std::lock_guard<std::mutex> lock(time_mut);
    if(held_since == 0) held_since = real_micros();
}

static void start_the_clock() {
    std::lock_guard<std::mutex> lock(time_mut);
    if(held_since != 0) held_total += real_micros() - held_since;
    held_since = 0;
}

/** (control_mut held) bring `armed` and `sdb_attention` in line with what is wanted */
static void refresh_watching() {
    auto *places = new armed_places();
    for(breakpoint &point : breakpoints) {
        if(!point.on) continue;
        for(code_place &place : point.places) {
            (*places)[place.function->bytecode + place.pc] = point.number;
        }
    }
    for(code_place &place : once_places) {
        // (a breakpoint on the same instruction is the better thing to report)
        places->emplace(place.function->bytecode + place.pc, ONCE);
    }

    const armed_places *before = armed.exchange(places);
    if(before != nullptr) old_armed.push_back(before);

    sdb_attention = (holding.load() || !places->empty() || stepping.load() != nullptr) ? 1 : 0;
}

// ---------------------------------------------------------------------------------
// The program's side
// ---------------------------------------------------------------------------------

/** has the fiber being stepped gone as far as the step says? (it is about to run the instruction at task->pc) */
static bool step_is_done(fiber *task) {
    Int depth = task->callFramePtr;

    switch(step_wanted) {
        case STEP_INTO_INSTRUCTION:
            return true;
        case STEP_NEXT_INSTRUCTION:
            return depth <= step_depth;                 // not inside what the instruction called
        case STEP_OUT:
            return depth < step_depth;
        case STEP_NEXT:
            if(depth > step_depth) return false;        // inside something the line called
            // fall through: back at (or below) where it was, the question is the same as for `into`
        case STEP_INTO:
            if(depth != step_depth || task->current != step_function) return true;
            return line_of(task->current, task->pc - task->current->bytecode) != step_line;
        default:
            return false;
    }
}

/** wait here until the program is let go; `why` is this thread's own reason to stop (STOP_NONE: it has none) */
static void wait_until_let_go(sharp_thread *thread, fiber *task, stop_reason why, int hit) {
    // from here the thread touches no objects: the collector need not wait for it
    tgc_enter_safe(thread);
    {
        std::unique_lock<std::mutex> lock(control_mut);

        stop_report mine;
        mine.thread = thread;
        mine.task = task;
        mine.reason = why;
        mine.breakpoint = hit;

        if(current_stop.thread == nullptr) {
            // the first to stop: the one the prompt reports. (A thread without a reason of its
            // own is first only when the program is held from its start, or was paused.)
            if(why == STOP_NONE) mine.reason = pause_wanted.load() ? STOP_PAUSED : STOP_LOADED;
            pause_wanted = false;
            current_stop = mine;

            holding = true;
            stop_the_clock();
            stepping = nullptr;         // a stop ends the step, whoever stopped
            once_places.clear();
            refresh_watching();
        } else if(why != STOP_NONE) {
            // another thread stopped first, and this one has a reason of its own: reported next
            later_stops.push_back(mine);
        }

        held_threads.push_back(thread);
        control_changed.notify_all();
        for(;;) {
            control_changed.wait(lock, [] { return !holding.load() || work_wanted != nullptr; });
            if(work_wanted == nullptr) break;

            // work for a thread of the program (sdb_run_on_held_thread): this one takes it. For as
            // long as it works it is a thread that touches objects, like any running one.
            const std::function<void()> *work = work_wanted;
            work_wanted = nullptr;
            lock.unlock();
            tgc_leave_safe(thread);
            (*work)();
            tgc_enter_safe(thread);
            lock.lock();
            work_done = true;
            control_changed.notify_all();
        }
        held_threads.erase(std::remove(held_threads.begin(), held_threads.end(), thread), held_threads.end());
    }
    // (not with control_mut held: this waits for a running collection to finish)
    tgc_leave_safe(thread);
}

void sdb_before_instruction(sharp_thread *thread, fiber *task) {
    stop_reason why = STOP_NONE;
    int hit = 0;

    // this thread's own reason to stop, if it has one - looked for also while the program is being
    // held for another thread: once it is let go, this instruction runs without coming here again
    const armed_places *places = armed.load();
    if(places != nullptr) {
        auto found = places->find(task->pc);
        if(found != places->end()) {
            hit = found->second;
            why = hit == ONCE ? STOP_AT_START : STOP_AT_BREAKPOINT;
        }
    }
    if(why == STOP_NONE && stepping.load() == task && step_is_done(task)) why = STOP_AFTER_STEP;

    if(why == STOP_NONE && !holding.load()) return;
    wait_until_let_go(thread, task, why, hit);
}

bool sdb_keeps_fiber(sharp_thread *thread) {
    fiber *stepped = stepping.load();
    return stepped != nullptr && thread->task == stepped;
}

// ---------------------------------------------------------------------------------
// The prompt's side
// ---------------------------------------------------------------------------------

std::vector<sharp_thread*> sdb_all_threads() {
    std::vector<sharp_thread*> all;

    // the new ones FIRST, then the scheduler's list: the monitor moves a thread from the first to
    // the second, so in this order one that moves in between is seen at least once
    {
        std::lock_guard<std::recursive_mutex> lock(unsched_mutex);
        for(_unsched_thread *fresh = unsched_threads; fresh != nullptr; fresh = fresh->next) all.push_back(fresh->thread);
    }
    {
        std::lock_guard<std::recursive_mutex> lock(thread_mutex);
        for(_sched_thread *listed = get_sched_threads(); listed != nullptr; listed = listed->next) {
            if(std::find(all.begin(), all.end(), listed->thread) == all.end()) all.push_back(listed->thread);
        }
    }
    return all;
}

void sdb_hold_when_loaded() {
    std::lock_guard<std::mutex> lock(control_mut);
    holding = true;
    stop_the_clock();
    sdb_attention = 1;
}

/**
 * Is no thread running Sharp code any more? Every thread of the program is
 * then waiting in the hook, or somewhere it touches no objects (asleep,
 * without a fiber, in a C++ call that waits): `gcSafe` says both, it is what
 * the collector goes by. (The collector's own thread and the idle thread never
 * run Sharp code; their gcSafe is always set.)
 */
static bool every_thread_is_still() {
    for(sharp_thread *thread : sdb_all_threads()) {
        if(thread->id == gc_threadid || thread->id == idle_threadid) continue;      // the VM's own: they run no Sharp code
        if(thread->state == THREAD_KILLED || thread->exited || thread->state == THREAD_CREATED) continue;
        if(__atomic_load_n(&thread->gcSafe, __ATOMIC_SEQ_CST) == 0) return false;
    }
    return true;
}

#define MOST_MILLIS_TO_SETTLE 2000

static bool every_thread_is_still();

/**
 * Ctrl+C. A signal handler may do very little: this one only sets flags - the
 * same ones a stop sets, so every thread waits at its next instruction - and
 * the prompt's thread, which looks every few milliseconds, does the rest.
 */
static void interrupted(int) {
    std::signal(SIGINT, interrupted);       // (some systems forget the handler once it has run)
    if(pause_wanted.load()) _exit(130);     // asked twice and nothing stopped: the way out
    if(holding.load()) return;              // the program is stopped already: the prompt is open
    pause_wanted = true;
    holding = true;
    sdb_attention = 1;
}

void sdb_pause() {
    if(holding.load()) return;
    pause_wanted = true;
    holding = true;
    sdb_attention = 1;
}

void sdb_pause_on_interrupt() {
    std::signal(SIGINT, interrupted);
}

/** the fiber with the lowest id that has started, into `*(fiber**)into` */
static void keep_first_fiber(fiber *task, void *into) {
    fiber **first = (fiber**)into;
    if(task->current == nullptr || task->pc == nullptr || task->state == FIB_KILLED) return;
    if(*first == nullptr || task->id < (*first)->id) *first = task;
}

/**
 * (control_mut held) A pause that no thread answered: none of them is running
 * Sharp code. So that there is something to look at, the stop is reported on
 * the program's first fiber - the main thread's own, where `main` runs. (A
 * thread whose fiber sleeps runs no fiber at all, so it is the fiber that is
 * looked for, not what a thread runs.) False when there is no fiber to show.
 */
static bool pause_without_a_thread() {
    fiber *first = nullptr;
    sched_visit_fibers(keep_first_fiber, &first);
    if(first == nullptr) return false;

    current_stop = stop_report();
    current_stop.reason = STOP_PAUSED;
    current_stop.thread = first->attachedThread != nullptr ? first->attachedThread : get_main_thread();
    current_stop.task = first;
    current_stop.atInstruction = false;
    pause_wanted = false;
    stop_the_clock();
    stepping = nullptr;
    refresh_watching();
    return true;
}

bool sdb_wait_for_stop(stop_report &report) {
    {
        std::unique_lock<std::mutex> lock(control_mut);
        int millisPaused = 0;

        // (looked at every few milliseconds, not only when told: a signal handler cannot tell)
        while(current_stop.thread == nullptr && !ended) {
            control_changed.wait_for(lock, std::chrono::milliseconds(10));
            if(current_stop.thread != nullptr || ended || !pause_wanted.load()) continue;

            // a pause is wanted: a thread that runs Sharp code answers within a moment. When
            // none has, and none of them is running Sharp code, nobody will
            millisPaused += 10;
            if(millisPaused < 200) continue;
            lock.unlock();
            bool still = every_thread_is_still();
            lock.lock();
            if(still && current_stop.thread == nullptr) pause_without_a_thread();
        }

        if(current_stop.thread == nullptr) return false;
        report = current_stop;
    }

    // the other threads stop at their next instruction: give them the moment that takes. A thread
    // busy inside C++ for longer than this is left running; `threads` says so.
    for(int waited = 0; waited < MOST_MILLIS_TO_SETTLE && !every_thread_is_still(); waited++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return true;
}

bool sdb_run_on_held_thread(const std::function<void()> &work) {
    std::unique_lock<std::mutex> lock(control_mut);
    if(held_threads.empty()) return false;

    work_wanted = &work;
    work_done = false;
    control_changed.notify_all();
    control_changed.wait(lock, [] { return work_done; });
    return true;
}

bool sdb_is_held(sharp_thread *thread) {
    std::lock_guard<std::mutex> lock(control_mut);
    return std::find(held_threads.begin(), held_threads.end(), thread) != held_threads.end();
}

void sdb_resume(step_kind step, fiber *task) {
    std::lock_guard<std::mutex> lock(control_mut);

    if(step != STEP_NONE && task != nullptr) {
        step_wanted = step;
        step_function = task->current;
        step_depth = task->callFramePtr;
        step_line = line_of(task->current, task->pc - task->current->bytecode);
        stepping = task;
    }

    if(!later_stops.empty()) {
        // another thread stopped for a reason of its own while this stop was looked at:
        // nothing runs, that stop is the next one reported
        current_stop = later_stops.front();
        later_stops.erase(later_stops.begin());
        refresh_watching();
        control_changed.notify_all();
        return;
    }

    current_stop = stop_report();
    holding = false;
    start_the_clock();
    refresh_watching();
    control_changed.notify_all();
}

void sdb_stop_once_at(const std::vector<code_place> &places) {
    std::lock_guard<std::mutex> lock(control_mut);
    once_places = places;
    refresh_watching();
}

void sdb_program_ended(int exitCode) {
    std::lock_guard<std::mutex> lock(control_mut);
    ended = true;
    exit_code = exitCode;
    control_changed.notify_all();
}

int sdb_exit_code() {
    std::lock_guard<std::mutex> lock(control_mut);
    return exit_code;
}

int sdb_add_breakpoint(const std::vector<code_place> &places, const string &written) {
    std::lock_guard<std::mutex> lock(control_mut);

    breakpoint point;
    point.number = ++last_number;
    point.places = places;
    point.written = written;
    breakpoints.push_back(point);

    refresh_watching();
    return point.number;
}

bool sdb_switch_breakpoint(int number, bool on) {
    std::lock_guard<std::mutex> lock(control_mut);
    for(breakpoint &point : breakpoints) {
        if(point.number == number) {
            point.on = on;
            refresh_watching();
            return true;
        }
    }
    return false;
}

bool sdb_clear_breakpoint(int number) {
    std::lock_guard<std::mutex> lock(control_mut);
    for(size_t i = 0; i < breakpoints.size(); i++) {
        if(breakpoints[i].number == number) {
            breakpoints.erase(breakpoints.begin() + i);
            refresh_watching();
            return true;
        }
    }
    return false;
}

void sdb_clear_all_breakpoints() {
    std::lock_guard<std::mutex> lock(control_mut);
    breakpoints.clear();
    refresh_watching();
}

std::vector<breakpoint> sdb_breakpoints() {
    std::lock_guard<std::mutex> lock(control_mut);
    return breakpoints;
}

// ---------------------------------------------------------------------------------
// Looking at a held fiber
// ---------------------------------------------------------------------------------

std::vector<code_place> call_stack_of(fiber *task) {
    std::vector<code_place> calls;
    if(task == nullptr || task->current == nullptr || task->pc == nullptr) return calls;   // (it has not started)

    code_place here;
    here.function = task->current;
    here.pc = task->pc - task->current->bytecode;
    calls.push_back(here);

    // frames[n] is what to go back to when the n-th call returns: the caller and
    // the instruction it was at. frames[0] is the fiber's own start, not a caller.
    for(Int i = task->callFramePtr; i >= 1; i--) {
        code_place caller;
        caller.function = &vm.methods[task->frames[i].returnAddress];
        caller.pc = task->frames[i].pc - caller.function->bytecode;
        calls.push_back(caller);
    }
    return calls;
}
