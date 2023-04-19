//
// Created by bknun on 9/18/2022.
//

#ifndef SHARP_MANIFEST_H
#define SHARP_MANIFEST_H

#include "../../../stdimports.h"

struct manifest {
    manifest()
    :
        functionPointers(0),
        application(""),
        version(""),
        debug(0),
        entryMethod(0),
        methods(0),
        classes(0),
        fvers(0),
        target(0),
        sourceFiles(0),
        strings(0),
        constants(0),
        threadLocals(0)
    {}

    manifest(const manifest &m)
    :
        functionPointers(m.functionPointers),
        application(m.application),
        version(m.version),
        debug(m.debug),
        entryMethod(m.entryMethod),
        methods(m.methods),
        classes(m.classes),
        fvers(m.fvers),
        target(m.target),
        sourceFiles(m.sourceFiles),
        strings(m.strings),
        constants(m.constants),
        threadLocals(m.threadLocals)
    {}


    Int functionPointers;
    string application;
    string version;
    bool debug;
    Int entryMethod;
    Int methods;
    Int classes;
    Int fvers;
    Int target;
    uInt sourceFiles;
    Int strings;
    Int constants;
    Int threadLocals;
};

#endif //SHARP_MANIFEST_H
