//
// Created by BraxtonN on 2/15/2018.
//

#ifndef SHARP_MANIFEST_H
#define SHARP_MANIFEST_H

#include "../../stdimports.h"
#include "oo/string.h"
#include "List.h"


/**
 * Application info
 */
struct Manifest {
    native_string executable;
    native_string application;
    native_string version;
    bool debug;
    int64_t entryMethod;
    int64_t methods, classes;
    int fvers;
    int target;
    long sourceFiles;
    int64_t strings;
};

extern Manifest manifest;

struct source_file{
    long id;
    List<native_string> source_line;

    void free() {
        for(unsigned int i = 0; i < source_line.size(); i++)
            source_line.get(i).free();
        source_line.free();
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
            sourceFiles()
    {
    }

    List<source_file> sourceFiles;

    std::string getLine(long line, long sourceFile);
    bool hasLine(long line, long sourceFile);
    void free();
};


#endif //SHARP_MANIFEST_H
