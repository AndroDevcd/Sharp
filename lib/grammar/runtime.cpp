//
// Created by BraxtonN on 1/30/2017.
//
#include "runtime.h"

void runtime::interpret() {
    if(preprocess()) {

    }
}

bool runtime::preprocess() {
    for(parser* p : parsers) {
        // Evaluate all types in memory
    }

    return true;
}
