//
// Created by BraxtonN on 6/29/2018.
//

#include <csignal>
#include <iostream>

namespace
{
    volatile std::sig_atomic_t gSignalStatus;
}

void os_signal(int signal)
{
    gSignalStatus = signal;
}

void setupSigHandler() {
    std::signal(SIGINT, os_signal);
    std::signal(SIGSEGV, os_signal);
    std::signal(SIGILL, os_signal);
    std::signal(SIGTERM, os_signal);
    std::signal(SIGABRT, os_signal);
}