//
// Created by BraxtonN on 2/17/2017.
//

#include <cstring>
#include <cstdio>
#include "../../stdimports.h"
#include "startup.h"
#include "../util/file.h"
#include "interp/vm.h"
#include "internal/Environment.h"
#include "internal/Exe.h"
#include "internal/Thread.h"

options c_options;
int __vinit(string e, list<string> pArgs);

void version() {
    cout << progname << " " << progvers;
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
    if (argc < 2) { // We expect at least 1 argument: the executable
        help();
        return 1;
    }

    string executable ="";
    list<string> pArgs;
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
            executable = argv[i++];
            while(i < argc)
                pArgs.push_back(string(argv[i++]));
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

int __vinit(string exe, list<string> pArgs) {
    if(CreateSharpVM(exe, pArgs) != 0) {
        fprintf(stderr, "Sharp VM init failed (check log file)\n");
        goto bail;
    }

    updateStackFile("Starting interpreter");
    //vm->InterpreterThreadStart(element_at(*Thread::threads, 0));

    return vm->exitVal;

    bail:
        if(vm != NULL) {
            jobIndx = 0;
            updateStackFile("shutting down sharp VM");
            pushStackDump();

            vm->DestroySharpVM();
        }

        return 1;
}
