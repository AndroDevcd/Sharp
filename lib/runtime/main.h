//
// Created by BraxtonN on 2/15/2018.
//

#ifndef SHARP_MAIN_H
#define SHARP_MAIN_H

#include "../../stdimports.h"

int runtimeStart(int argc, const char* argv[]);

void error(string message);

#define progname "sharp"
#define rev "r5"
#define progvers "2.3.63" rev

#ifdef SHARP_PROF_
enum profilerSort {
    tm,
    avgt,
    calls,
    ir
};
#endif

struct options {

    bool debugMode = false;

    /**
     * JIT Compiler enabled by default to boost Sharp
     * speeds of 25-50%+ faster than the original 
     * program speed
     */
    bool jit = true;

    /**
     * Slow boot allows you to not care about the initial startup time
     * gained by not compiling all the user code. This option will force
     * the JIT to imediatley compile all functions at startup-time to allow
     * for faster processing at runtime once completed.
     */
    bool slowBoot = true;

#ifdef SHARP_PROF_
    int sortBy = profilerSort::tm;
#endif
};

extern options c_options;

#endif //SHARP_MAIN_H
