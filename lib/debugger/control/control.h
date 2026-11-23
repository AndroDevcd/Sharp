//
// HOLDING THE PROGRAM AND LETTING IT GO.
//
// Two sides meet here:
//
//   THE PROGRAM'S THREADS run the interpreter. Before every instruction the
//   hook (hooks.h) asks this file whether to go on. A thread that must not go
//   on WAITS INSIDE THE HOOK - between two instructions, where everything it
//   was doing is on its fiber's stack and nothing is half done.
//
//   THE PROMPT'S THREAD (prompt/prompt.cpp) waits until a thread reports that
//   it stopped, lets a person look around, and lets the program go again.
//
// A thread that waits here keeps the state it had: nothing in sharp_thread or
// fiber is written. To the garbage collector it counts as a thread that touches
// no objects (as one blocked on the keyboard does), so collections go on.
//
// WHO IS HELD: every thread that runs Sharp code - the main thread and the ones
// the program started. The collector's thread, the idle thread and the
// scheduler's monitor never run the interpreter, so they never come here.
//
// THE WHOLE PROGRAM STOPS, NOT ONE THREAD. When one thread stops, every other
// thread stops at its next instruction; a thread that is not running Sharp code
// (it sleeps, waits for a fiber, sits in a C++ call) is left where it is and
// waits here only if it comes back. The prompt opens when every thread is
// either waiting here or not running Sharp code: sdb_wait_for_stop() waits for
// that, so what a person sees does not move under them.
//
// WHILE THE PROGRAM IS HELD THE SCHEDULER'S TIME STANDS STILL (hooks.h,
// SDB_SCHEDULER_TIME): no timer becomes due and no time slice runs out.
//
// TWO THREADS MAY HAVE A REASON TO STOP AT THE SAME MOMENT (two breakpoints).
// The first is reported; the other is reported when the first is let go, before
// anything runs again.
//
// A PAUSE (Ctrl+C) holds the program the same way, with no place in mind: the
// first thread to reach an instruction reports it. When no thread is running
// Sharp code at all, nobody comes; the prompt then opens on the main thread.
//
// WHY A THREAD STOPS (all three are looked for in the hook):
//   - the instruction is a breakpoint's
//   - the instruction is where a "run until here" ends (the program's `main`,
//     for the first stop)
//   - its fiber is being stepped and the step is done (see step_kind)
//

#ifndef SHARP_DEBUGGER_CONTROL_H
#define SHARP_DEBUGGER_CONTROL_H

#include "../../../stdimports.h"
#include "../symbols/symbols.h"
#include <functional>

struct sharp_thread;
class fiber;

enum stop_reason {
    STOP_NONE = 0,
    STOP_LOADED,                // before the very first instruction: the program is loaded, nothing has run
    STOP_AT_START,              // at the program's own `main`
    STOP_AT_BREAKPOINT,
    STOP_AFTER_STEP,
    STOP_PAUSED                 // a person asked (Ctrl+C)
};

/** why and where the program stopped: the thread that stopped first, and the fiber it was running */
struct stop_report {
    stop_reason reason = STOP_NONE;
    int breakpoint = 0;         // its number, for STOP_AT_BREAKPOINT
    sharp_thread *thread = nullptr;
    fiber *task = nullptr;
    // false for a pause that found NO thread running Sharp code (all of them sleep, wait or sit in
    // C++): `task` is then the program's first fiber, to have something to look at, and there is
    // nothing to step
    bool atInstruction = true;
};

struct breakpoint {
    int number = 0;                     // what a person calls it: counted from 1, never used twice
    std::vector<code_place> places;     // one, or several: every overload of a function, every function with code on a line
    string written;                     // as it was asked for (`notebook.add`, `notebook.sharp:41`)
    bool on = true;                     // false: kept, but the program does not stop at it (`disable`)
};

/**
 * How far a fiber goes when it is let go. A step is about ONE fiber - the one
 * that was stopped - wherever it runs; every other thread runs freely.
 *
 *      STEP_NEXT               until it is on another source line; the functions the line calls
 *                              run to their end first                              (`next`)
 *      STEP_INTO               until it is on another source line, or in another function (`into`)
 *      STEP_OUT                until the function it is in has returned            (`out`)
 *      STEP_NEXT_INSTRUCTION   one instruction; a call runs to its end: the next stop is the
 *                              instruction after it                                (`nexti`)
 *      STEP_INTO_INSTRUCTION   one instruction; a call is one: the next stop is the called
 *                              function's first                                    (`intoi`)
 */
enum step_kind {
    STEP_NONE = 0,
    STEP_INTO_INSTRUCTION,
    STEP_NEXT_INSTRUCTION,
    STEP_INTO,
    STEP_NEXT,
    STEP_OUT
};

// ---- the prompt's side

/**
 * From now on Ctrl+C does not end the program: it pauses it, as a breakpoint
 * would, wherever it is (STOP_PAUSED). Pressed again before the program has
 * stopped, it ends sdb - the way out when a thread is stuck inside C++.
 * Call it once the VM has set up its own handling of signals.
 */
void sdb_pause_on_interrupt();

/** stop the program wherever it is, as Ctrl+C does (STOP_PAUSED); nothing when it is held already */
void sdb_pause();

/** hold the program before its first instruction (call before it starts) */
void sdb_hold_when_loaded();

/** wait until the program stops; false when it ended instead */
bool sdb_wait_for_stop(stop_report &report);

/**
 * Let every held thread go on; `step` says how far `task` goes (STEP_NONE:
 * until something else stops it). When another thread stopped for a reason of
 * its own in the meantime, nothing runs: that stop is reported next.
 */
void sdb_resume(step_kind step = STEP_NONE, fiber *task = nullptr);

/**
 * Every thread there is - also one that was started a moment ago and that the
 * scheduler's monitor has not put on its list yet (it may already have run
 * into a breakpoint). The pointers stay good while the program is held.
 */
std::vector<sharp_thread*> sdb_all_threads();

/**
 * Run `work` on one of the program's own threads that wait in the hook, and
 * come back when it is done. Only a thread of the program may make objects
 * (each has its own store of them, and the collector goes by them), so this
 * is how the debugger makes one. False when no thread is held - a pause that
 * found every thread asleep.
 */
bool sdb_run_on_held_thread(const std::function<void()> &work);

/** is this thread waiting in the hook right now (it was running Sharp code when the program stopped)? */
bool sdb_is_held(sharp_thread *thread);

/** the next time any of these instructions is reached, stop there once (reported as STOP_AT_START) */
void sdb_stop_once_at(const std::vector<code_place> &places);

/** the program ran to its end (called by sdb's main once the VM comes back) */
void sdb_program_ended(int exitCode);
int sdb_exit_code();

/** a breakpoint at these places; gives its number */
int sdb_add_breakpoint(const std::vector<code_place> &places, const string &written);

/** switch a breakpoint on or off (it stays in the list either way); false when there is none with that number */
bool sdb_switch_breakpoint(int number, bool on);

/** take a breakpoint away; false when there is none with that number */
bool sdb_clear_breakpoint(int number);
void sdb_clear_all_breakpoints();

/** the breakpoints there are, in the order they were made */
std::vector<breakpoint> sdb_breakpoints();

// ---- looking at a held fiber

/**
 * The calls that led to where `task` is: the function it is in first, then
 * its caller, and so on. Empty for a fiber that has not started.
 */
std::vector<code_place> call_stack_of(fiber *task);

#endif //SHARP_DEBUGGER_CONTROL_H
