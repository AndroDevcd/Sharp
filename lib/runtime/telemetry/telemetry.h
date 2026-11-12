//
// Sharp VM Telemetry
// ==================
//
// Built-in performance instrumentation for the virtual machine. It answers
// "where did the time go?" without an external profiler: how long each thread
// spent in the interpreter vs. jit compiled code vs. waiting, how often we
// context switch and how long that takes, which opcodes are hot, which
// functions are hot, why a function could not be jit compiled, how much work
// the garbage collector did, and so on.
//
// HOW TO USE
//   1. Uncomment `#define SHARP_TELEMETRY` in stdimports.h (or build with
//      -DSHARP_TELEMETRY) and rebuild the runtime.
//   2. Run any program. When the VM exits it writes `sharp_telemetry.txt` into
//      the current working directory (override the path with the environment
//      variable SHARP_TELEMETRY_FILE).
//
// COST
//   When SHARP_TELEMETRY is NOT defined every macro below expands to nothing:
//   there is zero runtime cost and no extra code in the binary.
//   When it IS defined the interpreter gets a little slower (one counter
//   increment per instruction, two clock reads per mode change), so compare
//   numbers inside one report rather than against a non-telemetry build.
//
// HOW TO ADD A NEW METRIC
//   - a simple "how many times" counter: add a name to the TM_COUNTER list in
//     this file, then call TELEMETRY_COUNT(tm_your_counter) where it happens.
//     It shows up in the report automatically (per thread and in the totals).
//   - "how long did we spend doing X": add a name to the TM_MODE list and put
//     TELEMETRY_SCOPE(tm_mode_x); at the top of the C++ scope that does X.
//   Everything is recorded per OS thread without locks and merged when the
//   report is written.
//
// TIME ACCOUNTING MODEL
//   Each thread is always in exactly one "mode" (interpreter, jit, gc, ...).
//   TELEMETRY_SCOPE(mode) switches the mode for the lifetime of a C++ scope and
//   switches back when the scope ends, so nested work is charged to the
//   innermost scope only (exclusive time). Example: the interpreter calls a jit
//   compiled function which calls back into the interpreter - the time is split
//   interpreter / jit / interpreter and never counted twice.
//

#ifndef SHARP_TELEMETRY_H
#define SHARP_TELEMETRY_H

#include "../../../stdimports.h"

#ifdef SHARP_TELEMETRY

#include <chrono>

struct sharp_thread;
struct sharp_function;

// ---------------------------------------------------------------------------------
// Modes: what a thread is spending its time on (exclusive, see model above)
// ---------------------------------------------------------------------------------
#define TM_MODE_LIST(X) \
    X(tm_mode_other,        "vm runtime (other)",        "thread setup, call/return bookkeeping and anything not covered below") \
    X(tm_mode_interpreter,  "interpreter",               "executing bytecode in main_vm_loop") \
    X(tm_mode_jit,          "jit compiled code",         "executing machine code produced by the jit") \
    X(tm_mode_jit_compile,  "jit compiling",             "translating bytecode to machine code") \
    X(tm_mode_interrupt,    "system interrupts (INT)",   "exec_interrupt: io, time, threads, reflection, etc.") \
    X(tm_mode_native,       "native bridge calls",       "user native functions called through the bridge") \
    X(tm_mode_gc,           "garbage collection",        "mark & sweep (on the gc thread or on a thread forced to collect)") \
    X(tm_mode_sched_wait,   "waiting for a fiber",       "parked in observe_queue until the scheduler hands us a task") \
    X(tm_mode_suspended,    "suspended",                 "stopped by suspend_self (stop-the-world gc, thread suspend)") \
    X(tm_mode_sleep,        "sleeping",                  "SLEEP opcode / thread sleep")

enum telemetry_mode {
#define X(id, name, desc) id,
    TM_MODE_LIST(X)
#undef X
    tm_mode_count
};

// ---------------------------------------------------------------------------------
// Counters: how many times something happened
// ---------------------------------------------------------------------------------
#define TM_COUNTER_LIST(X) \
    X(tm_calls_total,            "function calls (prepare_method)") \
    X(tm_calls_into_jit,         "calls that ran jit compiled code") \
    X(tm_jit_loop_entries,       "hot loops that switched from the interpreter into jit code") \
    X(tm_calls_nested_interp,    "nested interpreter loops (called from jit/native code)") \
    X(tm_jit_exit_ok,            "jit function exits: finished normally") \
    X(tm_jit_exit_exception,     "jit function exits: uncaught exception") \
    X(tm_jit_exit_ctx_switch,    "jit function exits: context switch / unwound") \
    X(tm_jit_exit_kill,          "jit function exits: thread killed") \
    X(tm_interrupts,             "system interrupts executed (INT)") \
    X(tm_native_calls,           "native bridge calls") \
    X(tm_context_switches,       "context switches (thread parked its fiber)") \
    X(tm_suspends,               "times suspended (suspend_self)") \
    X(tm_exceptions_thrown,      "vm exceptions created") \
    X(tm_exceptions_caught,      "exceptions caught by a handler/finally block") \
    X(tm_objects_created,        "objects allocated") \
    X(tm_object_copies,          "reference assignments (no longer counted: copy_object is a plain store)") \
    X(tm_stack_grows,            "fiber stack grows (realloc)") \
    X(tm_frame_grows,            "fiber call-frame grows (realloc)") \
    X(tm_lock_acquired,          "object locks acquired") \
    X(tm_lock_contended,         "object locks contended (forced a context switch)") \
    X(tm_fibers_created,         "fibers created") \
    X(tm_threads_created,        "threads created") \
    X(tm_gc_cycles,              "gc collections run") \
    X(tm_gc_objects_freed,       "objects freed by the gc")

enum telemetry_counter {
#define X(id, name) id,
    TM_COUNTER_LIST(X)
#undef X
    tm_counter_count
};

// ---------------------------------------------------------------------------------
// Per-thread data block. One per OS thread, created on first use and kept until
// the report is written (also after the thread has exited).
// ---------------------------------------------------------------------------------
struct telemetry_thread {
    long long vmThreadId;                    // sharp thread id, -1 for helper threads
    char name[64];
    bool finished;

    unsigned long long startNs, endNs;       // lifetime of the thread
    telemetry_mode mode;                     // what it is doing right now
    unsigned long long modeStartNs;          // when it started doing that
    unsigned long long modeNs[tm_mode_count];     // exclusive time per mode
    unsigned long long modeEnters[tm_mode_count]; // how often each mode was entered

    unsigned long long counters[tm_counter_count];
    unsigned long long opcodes[256];         // interpreter instructions executed, by opcode

    // context switch quality
    unsigned long long switchWaitNs;         // total time between parking a fiber and running the next one
    unsigned long long switchWaitMaxNs;      // worst case of the above
};

inline unsigned long long telemetry_now() {
    return (unsigned long long) std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
}

telemetry_thread* telemetry_register_thread();   // slow path: creates the block for this thread
extern thread_local telemetry_thread* tm_self;

inline telemetry_thread* telemetry_self() {
    telemetry_thread* t = tm_self;
    return t ? t : telemetry_register_thread();
}

/**
 * Switches the calling thread to `mode` and charges the time since the last
 * switch to the mode it was in. Returns the previous mode so it can be restored.
 */
inline telemetry_mode telemetry_switch_mode(telemetry_mode mode) {
    telemetry_thread* t = telemetry_self();
    unsigned long long now = telemetry_now();
    telemetry_mode previous = t->mode;
    t->modeNs[previous] += now - t->modeStartNs;
    t->modeStartNs = now;
    t->mode = mode;
    return previous;
}

/**
 * RAII helper behind TELEMETRY_SCOPE: enter a mode now, go back to the previous
 * one when the C++ scope ends (return, exception, fallthrough - all covered).
 */
struct telemetry_scope {
    telemetry_mode previous;
    explicit telemetry_scope(telemetry_mode mode) {
        previous = telemetry_switch_mode(mode);
        tm_self->modeEnters[mode]++;
    }
    ~telemetry_scope() { telemetry_switch_mode(previous); }
};

/**
 * RAII helper behind TELEMETRY_CONTEXT_SWITCH: measures how long the thread was
 * parked between giving up one fiber and being handed the next.
 */
void telemetry_context_switch_wait(unsigned long long ns);
struct telemetry_switch_timer {
    unsigned long long start;
    telemetry_switch_timer() : start(telemetry_now()) {}
    ~telemetry_switch_timer() { telemetry_context_switch_wait(telemetry_now() - start); }
};

// events that carry more than a counter
void telemetry_init();                                         // call once at vm start, registers the report writer
void telemetry_thread_begin(sharp_thread* thread);             // vm_thread_entry start
void telemetry_thread_end();                                   // vm_thread_entry end
void telemetry_name_thread(const char* name);                  // for helper threads (gc, scheduler)
void telemetry_jit_compiled(sharp_function* fn, unsigned long long ns, size_t codeBytes);
void telemetry_jit_blocked(sharp_function* fn, int opcode);    // fn could not be compiled because of this opcode
void telemetry_jit_exit(int jitReturnCode);
void telemetry_write_report();

// ---------------------------------------------------------------------------------
// The macros the rest of the VM uses
// ---------------------------------------------------------------------------------
#define TM_CONCAT_(a, b) a##b
#define TM_CONCAT(a, b) TM_CONCAT_(a, b)

/** Charge the rest of this C++ scope to `mode` */
#define TELEMETRY_SCOPE(mode) telemetry_scope TM_CONCAT(_tm_scope_, __LINE__)(mode);
/** counter++ / counter += n for the calling thread */
#define TELEMETRY_COUNT(counter) telemetry_self()->counters[counter]++;
#define TELEMETRY_ADD(counter, n) telemetry_self()->counters[counter] += (n);
/** Interpreter only: cache the opcode table pointer once per main_vm_loop call... */
#define TELEMETRY_OPCODE_SETUP unsigned long long* _tm_opcodes = telemetry_self()->opcodes;
/** ...and bump one slot per executed instruction (used inside DISPATCH) */
#define TELEMETRY_OPCODE(op) _tm_opcodes[(op)]++;
/** Top of observe_queue: count the switch, charge the wait to "waiting for a fiber", time it */
#define TELEMETRY_CONTEXT_SWITCH TELEMETRY_COUNT(tm_context_switches) TELEMETRY_SCOPE(tm_mode_sched_wait) telemetry_switch_timer _tm_switch_timer;
/** Run arbitrary telemetry-only code */
#define TELEMETRY(code) code

#else // ------------------------------------------------------------ telemetry disabled

#define TELEMETRY_SCOPE(mode)
#define TELEMETRY_COUNT(counter)
#define TELEMETRY_ADD(counter, n)
#define TELEMETRY_OPCODE_SETUP
#define TELEMETRY_OPCODE(op)
#define TELEMETRY_CONTEXT_SWITCH
#define TELEMETRY(code)

#endif // SHARP_TELEMETRY

#endif //SHARP_TELEMETRY_H
