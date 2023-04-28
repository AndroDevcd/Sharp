//
// Created by bknun on 9/21/2022.
//

#include <csignal>
#include "sig_handler.h"

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
    }
    return "?";
}

void os_signal(int signal) {
    if(signal != SIGINT) {
        cerr << "(" << signalToString(signal) << ") found, please contact the language developer with the call stack below" << endl;
    } else {
#ifdef POSIX_
        reset_original_console_settings();
#endif
    }

    exit(1);
}

void setup_sig_handler() {
    std::signal(SIGINT, os_signal);
    std::signal(SIGSEGV, os_signal);
    std::signal(SIGILL, os_signal);
    std::signal(SIGTERM, os_signal);
    std::signal(SIGABRT, os_signal);
}