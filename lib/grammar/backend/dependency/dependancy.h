//
// Created by BNunnally on 8/31/2021.
//

#ifndef SHARP_DEPENDANCY_H
#define SHARP_DEPENDANCY_H

#include "../../../../stdimports.h"

struct sharp_file;
struct sharp_class;
struct sharp_module;

enum dependency_type {
    no_dependency,
    dependency_file,
    dependency_class
};

/**
 *
 * File dependencies:
 *  - Files
 *
 * class dependencies: File Owner -> files holding each class
 *  - Classes
 *
 * Field dependencies: Class Owner -> classes holding each item
 *  - Classes
 *  - Fields
 *  - Functions
 *
 * Function Dependencies: Class Owner -> classes holding each item
 *  - Fields
 *  - Functions
 *  - Classes
 *
 */
struct dependency {
    dependency()
    :
        fileDependency(NULL),
        classDependency(NULL),
        type(no_dependency)
    {}

    dependency(const dependency &d)
            :
            fileDependency(d.fileDependency),
            classDependency(d.classDependency),
            type(d.type)
    {}

    sharp_file *fileDependency;
    sharp_class *classDependency;
    dependency_type type;
};

void create_dependency(sharp_class* depender, sharp_file* dependee);

sharp_class* resolve_class(sharp_module*, string, bool, bool);
sharp_class* resolve_class(sharp_file*, string, bool, bool);
sharp_class* resolve_class(string, bool, bool);

#endif //SHARP_DEPENDANCY_H
