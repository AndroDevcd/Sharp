//
// Created by bknun on 9/12/2017.
//

#ifndef SHARP_RUNTIME_H
#define SHARP_RUNTIME_H

#include "../../stdimports.h"
#include "List.h"

class Interpreter {

};

#define progname "bootstrap"
#define progvers "0.2.4"

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
     * Easter egg to enable debug mode
     *
     * Allows you to see a little information
     * on what the compiler is doing
     */
    bool debugMode = false;

    /*
     * Dump object code
     */
    bool objDump = false;

    /*
     * Maximum errors the compiler will allow
     */
    unsigned long error_limit = 1000;

    /*
     * Machine platform target to run on
     */
    int target = versions.ALPHA;
};

int _bootstrap(int argc, const char* argv[]);
void rt_error(string message);
void printVersion();
std::string to_lower(string s);
bool all_integers(string int_string);
void exec_runtime(List<string>& files);

extern options c_options;
#define opt(v) strcmp(argv[i], v) == 0

#endif //SHARP_RUNTIME_H
