//
// Created by BraxtonN on 2/17/2017.
//

#ifndef SHARP_STARTUP_H
#define SHARP_STARTUP_H

#include "../../stdimports.h"

int runtimeStart(int argc, const char* argv[]);

void error(string message);

#define progname "sharp"
#define rev "r2"
#define progvers "0.2.24" rev

struct options {

};

extern options c_options;

template <class T>
inline T& element_at(list<T>& l, size_t x) {
    return *std::next(l.begin(), x);
}

template <class T>
inline T& element_at(list<T>& l, T search) {
    for(T var : l) {
        if(search == var)
            return var;
    }

    return *std::next(l.begin(), 0);
}

template <class T>
inline long element_index(list<T>& l, T search) {
    long iter=0;
    for(T var : l) {
        if(search == var)
            return iter;
        iter++;
    }

    return -1;
}

template <class T>
inline bool element_has(list<T>& l, T search) {
    for(T var : l) {
        if(search == var)
            return true;
    }
    return false;
}

#endif //SHARP_STARTUP_H
