//
// THE THREADS AND FIBERS OF THE HELD PROGRAM, AND WHAT EACH WAS DOING.
//
// A THREAD is an OS thread that runs Sharp code; a FIBER is one flow of the
// program (a coroutine, or a thread's own first fiber) with its own stack. A
// thread runs one fiber at a time; a fiber that waits - for a timer, a lock,
// its turn - is on no thread at all. So "where is the program" has two
// answers, and sdb lists both.
//
// THE STATE SHOWN IS THE ONE THE THREAD OR FIBER HAD when the program stopped.
// Holding the program writes nothing into them (control.h), so a thread that
// was asleep is shown asleep, not "stopped by the debugger":
//
//      a thread    running                 it was executing Sharp code (it waits in the hook now)
//                  extern c++              inside the VM's C++ or a native function, waiting: reading
//                                          the keyboard, a socket, waiting to join a thread
//                  extern c++ (busy)       inside C++ and still running: it did not stop
//                  waiting for a fiber     it has no fiber that can run (all of its fibers wait)
//                  suspended               the program suspended it
//                  not started / finished
//                  VM                      one of the VM's own: the collector's thread, the idle thread - never held
//
//      a fiber     running on thread N     (the thread's state says the rest)
//                  ready                   it can run and waits for a thread to take it
//                  sleeping                in delay(): the time left is the scheduler's, which
//                                          stands still while the program is held
//                  waiting for a lock      ... and which fiber holds it
//                  suspended               by fiber.suspend(), until it is woken
//                  not started / finished
//
// Rows are made while the scheduler's locks are held and hold only copies and
// ids, so the commands that print them need no lock. The `task` of a row is
// the fiber itself, for looking into; it stays good while the program is held.
//

#ifndef SHARP_DEBUGGER_THREADS_H
#define SHARP_DEBUGGER_THREADS_H

#include "../../runtime/stdimports.h"
#include <vector>

struct sharp_thread;
class fiber;

struct thread_row {
    Int id = 0;
    string name;
    string state;               // in words, as in the table above
    bool own = false;           // one of the VM's own threads, not the program's
    fiber *task = nullptr;      // the fiber it runs (null: none)
    Int fiberId = -1;
    string place;               // where that fiber is (see fiber_row.place); "" when it runs none
};

struct fiber_row {
    Int id = 0;
    string name;
    string state;
    Int threadId = -1;          // the thread it runs on; -1: none
    Int boundTo = -1;           // the only thread that may run it; -1: any
    // where it is: the nearest of its calls that is not the library's (`app#fetch  net.sharp:52`) -
    // a fiber that sleeps or waits is always somewhere inside the library, which says little.
    // The innermost call when all of them are the library's; "" when it has not started.
    string place;
    string calls;               // every call that led there, in one line: what `fibers <word>` looks through
    fiber *task = nullptr;
};

/** every thread there is, by id */
std::vector<thread_row> threads_now();

/** every fiber there is, by id */
std::vector<fiber_row> fibers_now();

#endif //SHARP_DEBUGGER_THREADS_H
