//
// Created by BraxtonN on 10/6/2019.
//

#ifndef SHARP_OPTIONS_H
#define SHARP_OPTIONS_H

#include "../../stdimports.h"
#include "List.h"

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
     * Obfuscate everything that is compiled.
     */
    bool obfuscate = false;

    /*
     * Obfuscation map file to output.
     */
    string mapFile = "mapping.txt";

    /*
     * Only compile all the files
     */
    bool compile = false;

    /*
     * Output file to write to
     */
    string out = "out";

    /*
     * Output file to write to
     */
    string nativeCodeDir = "generated";

    /*
     * Application version
     */
    string vers = "1.0";

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
    bool asmDump = false;

    /*
     * Maximum errors the compiler will allow
     */
    uInt error_limit = 1000;

    /*
     * Machine platform target to run on
     */
    int target = alpha_cr4;

    /*
     * List of user defined library paths
     */
    List<string> libraries;
};

extern struct options c_options;

// ASM DUMP SWITCHES
#define ASM_DEMANGLE   0
#define ASM_SYNTAX     1
#define ASM_DEBUG_INFO 2

// WARNING SWITCHES
#define __WGENERAL 0
#define __WACCESS 1
#define __WAMBIG 2
#define __WDECL 3
#define __WMAIN 4
#define __WCAST 5
#define __WINIT 6

extern bool warning_map[];

#endif //SHARP_OPTIONS_H
