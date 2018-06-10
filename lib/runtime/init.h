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
#define progvers "2.2.130" rev

struct options {

    bool debugMode = true;
};

extern options c_options;

#endif //SHARP_INIT_H
