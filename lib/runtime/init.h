//
// Created by BraxtonN on 2/15/2018.
//

#ifndef SHARP_INIT_H
#define SHARP_INIT_H

#include "../../stdimports.h"

int runtimeStart(int argc, const char* argv[]);

void error(string message);

#define progname "sharp"
#define rev "r5"
#define progvers "2.2.157" rev

#ifdef SHARP_PROF_
enum profilerSort {
    tm,
    avgt,
    calls,
    ir
};
#endif

struct options {

    bool debugMode = true;

#ifdef SHARP_PROF_
    int sortBy = profilerSort::tm;
#endif
};

extern options c_options;

#endif //SHARP_INIT_H
