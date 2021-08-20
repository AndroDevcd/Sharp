//
// Created by BraxtonN on 2/15/2018.
//

#ifndef SHARP_MAIN_H
#define SHARP_MAIN_H

#include "../../stdimports.h"

int runtimeStart(int argc, const char* argv[]);

void error(string message);

#define progname "sharp"
#define rev "r8"
#define progvers "3.0.0" rev

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

#ifdef SHARP_PROF_
    int sortBy = profilerSort::tm;
#endif
};

extern options c_options;

#endif //SHARP_MAIN_H
