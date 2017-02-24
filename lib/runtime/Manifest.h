//
// Created by BraxtonN on 2/17/2017.
//

#ifndef SHARP_MANIFEST_H
#define SHARP_MANIFEST_H

#include "../../stdimports.h"

/**
 * Application info
 */
struct Manifest {

    string application;
    string version;
    bool debug;
    int64_t entry;
    int64_t methods, classes;
    int fvers;
    int target;
    int64_t isize;
    int64_t strings;
};


#endif //SHARP_MANIFEST_H
