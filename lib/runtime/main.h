//
// Created by bknun on 9/16/2022.
//

#ifndef SHARP_MAIN_H
#define SHARP_MAIN_H

#include "../../stdimports.h"

int str_start(int argc, const char* argv[]);

void error(string message);

#define progname "sharp"
#define rev "r9"
#define progvers "3.0.0" rev

struct options {
    bool debugMode = true;
};

extern options c_options;
extern string executable;
extern std::list<string> exeArgs;

#endif //SHARP_MAIN_H
