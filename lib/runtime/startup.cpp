//
// Created by BraxtonN on 2/17/2017.
//

#include <cstring>
#include "../../stdimports.h"
#include "startup.h"
#include "../util/file.h"

options c_options;
int __vinit(string e);

void version() {

}

void error(string message) {
    cout << "sharp:  error: " << message << endl;
    exit(1);
}

void help() {
    std::cerr << "Usage: sharp" << " {OPTIONS} EXECUTABLE" << std::endl;
    cout << "Source file must have a .sharp extension to be compiled\n" << endl;
    cout << "[-options]\n\n    -V                print the bootstrap version number and exit" << endl;
    cout <<               "    -showversion      print the bootstrap version number and continue." << endl;
    cout <<               "    --h -?            display this help message." << endl;
}

#define opt(v) strcmp(argv[i], v) == 0

int runtimeStart(int argc, const char* argv[])
{
    if (argc < 2) { // We expect 2 arguments: the source file(s), and program options
        help();
        return 1;
    }

    string executable ="";
    for (int i = 1; i < argc; ++i) {
        if(opt("-V")){
            version();
            exit(0);
        }
        else if(opt("-h") || opt("-?")){
            help();
            exit(0);
        }
        else if(opt("-showversion")){
            version();
            cout << endl;
        }
        else if(string(argv[i]).at(0) == '-'){
            error("invalid option `" + string(argv[i]) + "`, try sharp -h");
        }
        else {
            // add the source files
            if(executable == "")
                executable = argv[i];
            else {
                error("multiple executables '" + executable + "' and '" + argv[i] + "'");
                break;
            }
            continue;
        }
    }

    if(executable == ""){
        help();
        return 1;
    }

    if(!file::exists(executable.c_str())){
        error("file `" + executable + "` doesnt exist!");
    }

    return __vinit(executable);
}

int __vinit(string exe) {
    return 0;
}
