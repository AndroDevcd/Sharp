//
// Created by BNunnally on 9/5/2020.
//

#ifndef SHARP_FIBER_H
#define SHARP_FIBER_H

#include "../../stdimports.h"
#include "symbols/string.h"
#include "profiler.h"

enum fiber_state {
    FIB_RUNNING,
    FIB_SUSPENDED,
    FIB_KILLED
};

class fiber {
public:
    static fiber* makeFiber(string name, Method* main);

    void free();

    static uInt fibId;
    uInt id;
    native_string name;
    Int stackLimit;
    Cache cache, pc;
    fiber_state state;
    int exitVal;
    Object exceptionObject;
    Method *main;
    Int calls;
    StackElement* dataStack, *sp, *fp;
    Method *current;
    Frame *callStack;
    double *registers;
};

#endif //SHARP_FIBER_H
