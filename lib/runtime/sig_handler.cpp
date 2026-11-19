//
// Created by bknun on 9/21/2022.
//

#include "cffi/c_bind.h"
#include <csignal>
#include "sig_handler.h"
#include "multitasking/thread/sharp_thread.h"
#include "error/vm_exception.h"
#include "termios.h"
#include "telemetry/telemetry.h"
#ifdef POSIX_
#include <unistd.h>
#endif

string signalToString(int signal) {
    switch(signal) {
        case SIGINT:
            return "SIGINT";
        case SIGSEGV:
            return "SIGSEGV";
        case SIGILL:
            return "SIGILL";
        case SIGTERM:
            return "SIGTERM";
        case SIGABRT:
            return "SIGABRT";
#ifdef POSIX_
        case SIGHUP:
            return "SIGHUP";
        case SIGQUIT:
            return "SIGQUIT";
#endif
    }
    return "?";
}

// set once a signal is being handled: a second one (the same key pressed again, or a crash while
// the first is being reported) ends the process at once instead of starting the report again
static volatile std::sig_atomic_t handling_a_signal = 0;

void os_signal(int signal) {
#ifdef POSIX_
    // FIRST, before anything that could fail: the console goes back to how the VM found it
    reset_original_console_settings();
    if(handling_a_signal) _exit(1);
#endif
    handling_a_signal = 1;

    // (a stack trace is for a crash: being asked to stop - Ctrl+C, a closed terminal, `kill` - is not one)
    bool asked_to_stop = signal == SIGINT || signal == SIGTERM;
#ifdef POSIX_
    asked_to_stop = asked_to_stop || signal == SIGHUP || signal == SIGQUIT;
#endif
    if(!asked_to_stop) {
        if(thread_self != NULL) {
            string st;
            fill_stack_trace(st);
            cerr << st << endl;
        }

        // a crash inside a C function the program called is the C side's (or the declaration's), not the VM's:
        // it is reported as that, with what the function is and what it was handed (cffi/c_bind.cpp)
        string in_c = thread_self != NULL ? c_describe_call(thread_self) : string();
        if(!in_c.empty()) cerr << "(" << signalToString(signal) << ") " << in_c << endl;
        else cerr << "(" << signalToString(signal) << ") found, please contact the language developer with the call stack below" << endl;
    }

#ifdef POSIX_
    // The process ends HERE, with _exit - not with exit().
    //
    // exit() also runs the destructors of every global object, and one of them is the scheduler's
    // condition variable (monitor_cv). The thread this handler interrupted is usually the very
    // thread waiting on it - the main thread sits in run_scheduler() most of the time - and
    // destroying a condition variable waits until nobody waits on it. So exit() waited for a
    // thread that could not go on until exit() returned: after Ctrl+C the program neither ran
    // nor ended. (C++: never call exit() in a signal handler; _exit() is the call made for it.)
    //
    // What exit() would have done that is still wanted is done by hand: the console is already
    // back, the telemetry report is written, and what the program printed is flushed.
    TELEMETRY(telemetry_write_report();)
    fflush(stdout);
    fflush(stderr);
    _exit(1);
#else
    exit(1);
#endif
}

void setup_sig_handler() {
#ifdef POSIX_
    // the console as it is now, before the program has touched it - what os_signal puts back. It is
    // put back at an ordinary end too (atexit): a program may end while a key is being read.
    get_original_console_settings();
    atexit(reset_original_console_settings);
    std::signal(SIGHUP, os_signal);                 // the terminal was closed
    std::signal(SIGQUIT, os_signal);                // Ctrl+backslash
#endif
    std::signal(SIGINT, os_signal);
    std::signal(SIGSEGV, os_signal);
    std::signal(SIGILL, os_signal);
    std::signal(SIGTERM, os_signal);
    std::signal(SIGABRT, os_signal);
}