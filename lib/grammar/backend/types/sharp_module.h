//
// Created by BNunnally on 8/31/2021.
//

#ifndef SHARP_SHARP_MODULE_H
#define SHARP_SHARP_MODULE_H

#include "../../../../stdimports.h"
#include "../../List.h"

struct sharp_class;

/**
 * Sharp Module
 *
 * Fields:
 * @field classes
 * This represents all the classes present in the current module
 * which can be used for resolving them later
 *
 */
struct sharp_module {
    sharp_module()
    :
        name(""),
        classes(),
        genericClasses()
    {}

    sharp_module(string &package)
            :
            name(package),
            classes(),
            genericClasses()
    {}


    sharp_module(const sharp_module &m)
            :
            name(m.name),
            classes(),
            genericClasses()
    {
        copy(m);
    }

    ~sharp_module() {
        free();
    }

    void free() {
        classes.free();
        genericClasses.free();
    }

    void copy(const sharp_module &m);

    string name;
    List<sharp_class*> classes;
    List<sharp_class*> genericClasses;
};

sharp_module* get_module(string &packageName);
sharp_module* create_module(string &packageName);


#endif //SHARP_SHARP_MODULE_H
