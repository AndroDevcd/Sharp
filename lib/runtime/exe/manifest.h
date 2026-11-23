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
        applicationId(0),
        methods(0),
        classes(0),
        fvers(0),
        target(0),
        sourceFiles(0),
        strings(0),
        constants(0),
        threadLocals(0),
        obfuscated(false),
        optimized(false)
    {}

    manifest(const manifest &m)
    :
        functionPointers(m.functionPointers),
        application(m.application),
        applicationId(m.applicationId),
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
        threadLocals(m.threadLocals),
        obfuscated(m.obfuscated),
        optimized(m.optimized)
    {}


    Int functionPointers;
    string application;
    string version;
    bool debug;
    Int applicationId;
    Int entryMethod;
    Int methods;
    Int classes;
    Int fvers;
    Int target;
    uInt sourceFiles;
    Int strings;
    Int constants;
    Int threadLocals;
    // not from the manifest itself: said by the debugger's section at the end of the file
    // (exe_processor.cpp, process_debugger_data), for the debugger
    bool obfuscated;        // the program's names were replaced (sharpc -obf)
    bool optimized;         // it was built with -dbg and -O: it carries no names of local variables
};

#endif //SHARP_MANIFEST_H
