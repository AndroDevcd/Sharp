//
// Created by BNunnally on 8/26/2021.
//

#include "settings.h"
#include "../../util/File.h"

settings options;

/**
 * This array represents the map of all the warning types that are enabled/disabled
 * once a warning is fired it will refrence this map to check if it should fired
 *
 * all warnings will always be enabled by default
 */
bool warning_options[] = {
        true,   // __w_general
        true,   // __w_access
        true,   // __w_ambig
        true,   // __w_decl
        true,   // __w_main
        true,   // __w_cast
        true,   // __w_init
        true,   // __w_null
        true    // __w_dep
};

int supportedLibVersions = 2;
string supported_versions[] = {
        "0.2.9",
        "0.3.0"
};

void warning(string message) {
    cout << PROG_NAME << ":  warning: " << message << endl;
}

void error(string message) {
    cout << PROG_NAME << ":  error: " << message;
    exit(1);
}

void printVersion() {
    cout << PROG_NAME << " v" << PROG_VERS << endl;
}

bool isNumber(string int_string) {
    for(char c : int_string) {
        if(!isdigit(c))
            return false;
    }
    return true;
}

void check_target() {
    switch(options.target) {
        case alpha_cr4: {
            return;
        }
        default: {
            stringstream err;
            err << "invalid platform target `" << options.target << "`";
            error(err.str());
        }
    }
}

string toLower(string s) {
    string newstr = "";
    for(char c : s) {
        newstr += tolower(c);
    }
    return newstr;
}

void enable_show_all_errors(bool enable) {
    options.aggressive_errors = enable;
}


void enable_compile_only(bool enable) {
    options.compile = enable;
}

void set_output_file(string outFile) {
    options.out = outFile;
}

void add_library_path(string path) {
    if(!File::exists(path.c_str())) {
        warning("library path: " + path + " dosen't exist!");
        return;
    }

    if(!(File::getFileAttrs(path) & FILE_DIRECTORY)) {
        warning("library path: " + path + " is not a directory");
        return;
    }

    string fullPath;
    File::resolvePath(path, fullPath);
    options.libraries.add(fullPath);
}

void enable_magic_mode(bool enable) {
    options.magic = enable;
}

void enable_app_debugging(bool enable) {
    options.debug = enable;
}

void enable_debug_mode(bool enable) {
    options.debug_mode = enable;
}

void enable_code_obfuscation(bool enable) {
    options.obfuscate = enable;
}

void set_target_platform(string target) {
    if(isNumber(target)) {
        options.target = stoi(target, nullptr, 0);
        check_target();
    } else {
        if(toLower(target) == "alpha-cr4") {
            options.target = alpha_cr4;
        }
        else {
            error("unknown platform target $target");
        }
    }
}

void set_target_threads(string target) {
    if(isNumber(target)) {
        options.target = stoi(target, nullptr, 0);
        if(options.target <= 0 || options.target >= 10) {
            stringstream ss;
            ss << "invalid target threads amount" << target;
            error(ss.str());
        }
    } else {
        error("target threads amount must be number");
    }
}

void enable_warnings(bool enable) {
    options.warnings = enable;
}

void enable_warning_type(string wtype, bool enable) {
    if(wtype == "-waccess") warning_options[__w_access] = enable;
    else if(wtype == "-wambig") warning_options[__w_ambig] = enable;
    else if(wtype == "-wdecl") warning_options[__w_decl] = enable;
    else if(wtype == "-wmain") warning_options[__w_main] = enable;
    else if(wtype == "-wcast") warning_options[__w_cast] = enable;
    else if(wtype == "-winit") warning_options[__w_init] = enable;
    else if(wtype == "-wnull") warning_options[__w_null] = enable;
    else if(wtype == "-wdep") warning_options[__w_dep] = enable;
    else error("invalid warning type: " + wtype);
}

void set_compilation_mode(compilation_mode mode) {
    options.compile_mode = mode;
}

void enable_green_mode(bool enable) {
    options.green_mode = enable;
}

void set_new_project_path(string path) {
    options.new_project_dir = path;
}

void set_source_version(string version) {
    options.version = version;
}

void add_ignored_file(string path) {
    if(!File::exists(path.c_str())) {
        warning("ignored file: " + path + " dosen't exist!");
        return;
    }

    if(File::getFileAttrs(path) & FILE_DIRECTORY) {
        warning("ignored file: " + path + " is a directory");
        return;
    }

    string fullPath;
    File::resolvePath(path, fullPath);
    options.ignored_files.add(fullPath);
}

void add_ignored_directory(string path) {
    if(!File::exists(path.c_str())) {
        warning("ignored directory: " + path + " dosen't exist!");
        return;
    }

    if(!(File::getFileAttrs(path) & FILE_DIRECTORY)) {
        warning("ignored directory: " + path + " is not a directory");
        return;
    }

    string fullPath;
    File::resolvePath(path, fullPath);
    options.ignored_directories.add(fullPath);
}

void set_project_directory(string path) {
    if(!File::exists(path.c_str())) {
        warning("project directory: " + path + " dosen't exist!");
        return;
    }

    if(!(File::getFileAttrs(path) & FILE_DIRECTORY)) {
        warning("project directory: " + path + " is not a directory");
        return;
    }

    string fullPath;
    File::resolvePath(path, fullPath);
    options.project_dir = fullPath;
    set_compilation_mode(project_mode);
}

void enable_warnings_as_errors() {
    enable_warnings(true);
    options.werrors = true;
}

void set_native_code_directory(string path) {
    if(File::exists(path.c_str())
       && !(File::getFileAttrs(path) & FILE_DIRECTORY)) {
        warning("native code directory: " + path + " is not a directory!");
        return;
    }

    options.native_code_dir = path;
}

void enable_source_obfuscation(bool enable) {
    options.obfuscate = enable;
}

void set_error_limit(string limit) {
    if(isNumber(limit)) {
        options.max_errors = strtoll(limit.c_str(), nullptr, 0);

        if(options.max_errors > 100000) {
            error("cannot set the max errors allowed higher than (100,000) - " + limit);
        } else if(options.max_errors < 10) {
            error("cannot have an error limit less than 10");
        }
    }
    else {
        error("invalid error limit set " + limit);
    }
}
