//
// Created by bknun on 9/12/2017.
//

#ifndef SHARP_RUNTIME_H
#define SHARP_RUNTIME_H

#include "../../stdimports.h"

class Runtime {

};

#define progname "bootstrap"
#define progvers "0.1.102"

struct options {
    ~options()
    {
        out.clear();
        vers.clear();
    }

    /*
     * Activate aggressive error reporting for the compiler.
     */
    bool aggressive_errors = false;

    /*
     * Only compile all the files
     */
    bool compile = false;

    /*
     * Output file to write to
     */
    string out = "out";

    /*
     * Application version
     */
    string vers = "1.0";

    /*
     * Disable warnings
     */
    bool warnings = true;

    /*
     * Optimize code
     */
    bool optimize = false;

    /*
     * Set code to be debuggable (flag only used in manifest creation)
     */
    bool debug = true;

    /*
     * Strip debugging info (if-applicable)
     */
    bool strip = false;

    /*
     * Enable warnings as errors
     */
    bool werrors = false;

    /*
     * Allow unsafe code
     */
    bool unsafe = false;

    /*
     * Easter egg to enable magic mode
     */
    bool magic = false;

    /*
     * Dump object code
     */
    bool objDump = false;

    /*
     * Machine platform target to run on
     */
    int target = versions.ALPHA;
};

int _bootstrap(int argc, const char* argv[]);

extern options c_options;


int _bootstrap(int argc, const char* argv[]);

#endif //SHARP_RUNTIME_H
