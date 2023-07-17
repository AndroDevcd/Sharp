//
// Created by BNunnally on 8/26/2021.
//

#ifndef SHARP_SETTINGS_H
#define SHARP_SETTINGS_H

#include "../../../stdimports.h"
#include "../compiler_info.h"
#include "../platform.h"
#include "../List.h"

enum optimization_level {
    no_optimization = 0,

    /*
    * This optimization consists of the most basic level of optimization where
    * no code is changed at the "user" level. Meaning all code written at the language
    * level remain in place and code at the virtual machine level is replaced with more high performance
    * instructions
    *
    */
    basic_optimization = 1,

    /*
    * This optimization consists of the highest level of optimization available for your code
    * Code under this optimization will be changed at both the "user" level and vm level. Meaning that code written at the language
    * level may not exist at the lower level/ may be heavily modified and code at the virtual machine level is replaced with more high performance
    * instructions
    *
    */
    high_performance_optimization = 2
};

enum compilation_mode {
    /*
    * This mode specifies that you must explicitly pass in
    * any files that you want compiled
    */
    file_mode = 0,

    /*
    * This mode specifies that you want to run the compiler in project mode,
    * signifying that you do not have to explicitly provide any files to be compiled.
    * Any additional files and or directories added to the options will also be added
    * to the compilation list as well.
    */
    project_mode = 1
};

struct settings {
    /*
     * Activate aggressive error reporting for the compiler.
     */
    bool aggressive_errors = false;

    /*
     * Only compile all the files
     */
    bool compile = false;

    /*
     * Obfuscate all classes, fields, and functions
     */
    bool obfuscate = false;

    /*
     * Output file to write to
     */
    string out = "out";

    /*
     * Output file to write to
     */
    string new_project_dir = "";

    /*
     * Location of the project you want to compile
     */
    string project_dir = "";

    /*
     * Name of the project being compiled
     */
    string project_name = "";


    /*
     * Mapping file to write to
     */
    string map_file = "mapping.txt";

    /*
     * Directory to which c++ code will be generated
     */
    string native_code_dir = "generated";

    /*
     * Application version
     */
    string version = "1.0";

    /*
     * Disable warnings
     */
    bool warnings = true;

    /*
     * Execute source code after successful compilation
     */
    bool run_source = false;

    /*
     * Set the compilation mode
     */
    int compile_mode = file_mode;

    /*
     * Set code to be debuggable (flag only used in manifest creation)
     */
    bool debug = true;

    /*
     * Enable warnings as errors
     */
    bool werrors = false;

    /*
     * Enable green mode for less overall power consumption
     * When compiling source code
     */
    bool green_mode = false;

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
    bool debug_mode = false;

    /*
     * Maximum errors the compiler will allow
     */
    Int max_errors = 1000;

    /*
     * Machine platform target to run on
     */
    int target = alpha;

    /*
     * List of user defined library paths
     */
    List<string> libraries;

    /*
     * List of ignored directories (only used in project mode)
     */
    List<string> ignored_directories;

    /*
     * List of ignored files
     */
    List<string> ignored_files;

    /*
     * List of source files to be compiled
     */
    List<string> source_files;
};

void enable_show_all_errors(bool);
void enable_compile_only(bool);
void set_output_file(string);
void add_library_path(string);
void enable_magic_mode(bool);
void enable_debug_mode(bool);
void enable_app_debugging(bool);
void set_target_platform(string);
void enable_warnings(bool);
void enable_warning_type(string, bool);
void set_compilation_mode(compilation_mode);
void enable_run_source_code(bool);
void enable_green_mode(bool);
void set_new_project_path(string);
void set_source_version(string);
void add_ignored_file(string);
void add_ignored_directory(string);
void set_project_directory(string);
void enable_warnings_as_errors();
void set_error_limit(string);
void set_native_code_directory(string);
void enable_source_obfuscation(bool);

void warning(string message);
void error(string message);
bool isNumber(string int_string);
void check_target();
string toLower(string s);
void printVersion();


// WARNING SWITCHES
#define __w_general 0
#define __w_access 1
#define __w_ambig 2
#define __w_decl 3
#define __w_main 4
#define __w_cast 5
#define __w_init 6
#define __w_null 7
#define __w_dep 8

#define SETTINGS_FILE "settings.json"

extern bool warning_options[];
extern string supported_versions[];
extern int supportedLibVersions;
extern settings options;

#endif //SHARP_SETTINGS_H
