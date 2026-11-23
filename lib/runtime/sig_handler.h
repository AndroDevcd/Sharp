//
// Created by bknun on 9/21/2022.
//

#ifndef SHARP_SIG_HANDLER_H
#define SHARP_SIG_HANDLER_H

#include "../../stdimports.h"

void setup_sig_handler();

//
// SIGNALS A PROGRAM ASKED TO HEAR ABOUT (the library: std.system signals).
//
// By default the three below end the program. A program that WATCHES one is not
// ended by it: the VM only counts that it came, and the program takes it when
// it is ready - on a thread of its own, at a place where nothing is half done.
// (The handler of a signal runs in the middle of whatever the thread was doing
// and may do almost nothing safely: it adds one to a counter, and that is all.)
//
//      watch_signal(which, on)     from now on, hear about it / let it end the program again
//      take_signal(millis)         the next one that came (0: none within that time)
//
// A program that does not answer must still be stoppable: a watched signal that
// came MOST_UNANSWERED_SIGNALS times without being taken ends the program as it
// always did. Crashes (a bad address, an illegal instruction) cannot be watched.
//
enum watched_signal {
    SIGNAL_INTERRUPT = 1,       // Ctrl+C
    SIGNAL_TERMINATE = 2,       // asked to end: `kill`, the system shutting down
    SIGNAL_HANGUP = 3,          // its terminal was closed (not on Windows)
    WATCHED_SIGNALS = 4
};
#define MOST_UNANSWERED_SIGNALS 3

void watch_signal(int which, bool on);
int take_signal(Int millis);

#endif //SHARP_SIG_HANDLER_H
