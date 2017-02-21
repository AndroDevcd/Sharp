//
// Created by BraxtonN on 2/17/2017.
//

#ifndef SHARP_STARTUP_H
#define SHARP_STARTUP_H

#include "../../stdimports.h"

int runtimeStart(int argc, const char* argv[]);

void error(string message);

#define progname "sharp"
#define rev "r1"
#define progvers "0.1.0" rev

struct options {

};

extern options c_options;

#endif //SHARP_STARTUP_H
