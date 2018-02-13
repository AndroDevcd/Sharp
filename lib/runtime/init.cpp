//
// Created by BraxtonN on 2/10/2018.
//
#include <cstring>
#include <cstdio>
#include "../../stdimports.h"
#include "../util/file.h"
#include "oo/string.h"
#include "List.h"

void help() {
    std::cerr << "Usage: sharp" << " {OPTIONS} EXECUTABLE" << std::endl;
    cout << "Source file must have a .sharp extension to be compiled\n" << endl;
    cout << "[-options]\n\n    -V                print the bootstrap version number and exit" << endl;
    cout <<               "    -showversion      print the bootstrap version number and continue." << endl;
    cout <<               "    --h -?            display this help message." << endl;
}

int __init(int argc, const char* argv[])
{
    if (argc < 2) { // We expect at least 1 argument: the executable
        help();
        return 1;
    }

    string executable ="";
    List<native_string> pArgs;
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
        }
        else if(string(argv[i]).at(0) == '-'){
            error("invalid option `" + string(argv[i]) + "`, try sharp -h");
        }
        else {
            // add the source files
            executable = argv[i++];
             native_string arg;
            while(i < argc) {
                arg = string(argv[i++]);
                pArgs.push_back();

                pArgs.get(pArgs.size()-1).init();
                pArgs.get(pArgs.size()-1) = arg;
            }
            arg.free();
            break;
        }
    }

    if(executable == ""){
        help();
        return 1;
    }

    if(!file::exists(executable.c_str())){
        error("file `" + executable + "` doesnt exist!");
    }

    return __vinit(executable, pArgs);
}
