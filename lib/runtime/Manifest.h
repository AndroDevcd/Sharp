//
// Created by BraxtonN on 2/15/2018.
//

#ifndef SHARP_MANIFEST_H
#define SHARP_MANIFEST_H

#include "../../stdimports.h"
#include "List.h"


/**
 * Application info
 */
struct Manifest {
public:
    Manifest()
    :
        application(),
        version()
    {
        init();
    }

    void init() {
        debug = false;
        entryMethod = 0;
        methods = 0;
        classes = 0;
        fvers = 0;
        target = 0;
        sourceFiles = 0;
        strings = 0;
        constants = 0;
        threadLocals = 0;
        functionPointers = 0;
    }

    Int functionPointers;
    string application;
    string version;
    bool debug;
    Int entryMethod;
    Int methods, classes;
    Int fvers;
    Int target;
    uInt sourceFiles;
    Int strings;
    Int constants;
    Int threadLocals;
};

struct SourceFile {
    string name;
    _List<string> lines;

    void init() {
        lines.init();
    }

    void free() {
        for(uInt i = 0; i < lines.size(); i++)
            lines.get(i).clear();
        lines.free();
    }
};

/**
 * File and debugging info, line
 * by line debugging meta data
 */
class Meta {

public:
    Meta()
            :
            files()
    {
    }

    void init() { Meta(); }

    _List<SourceFile> files;

    string& getLine(Int line, Int sourceFile);
    bool hasLine(Int line, Int sourceFile);
    void free();
};


#endif //SHARP_MANIFEST_H
