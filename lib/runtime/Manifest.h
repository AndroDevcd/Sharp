//
// Created by BraxtonN on 2/17/2017.
//

#ifndef SHARP_MANIFEST_H
#define SHARP_MANIFEST_H

#include "../../stdimports.h"

class Method;

/**
 * Application info
 */
struct Manifest {

    string executable;
    string application;
    string version;
    bool debug;
    int64_t entry;
    int64_t methods, classes;
    int fvers;
    int target;
    int64_t isize;
    int64_t strings;
    int64_t baseaddr;
    Method* main;
};


#endif //SHARP_MANIFEST_H
