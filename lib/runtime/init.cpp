//
// Created by BraxtonN on 2/10/2018.
//
#include <cstring>
#include <cstdio>
#include "../../stdimports.h"
#include "../util/file.h"
#include "oo/string.h"
#include "List.h"
#include "init.h"
#include "oo/Object.h"
#include "VirtualMachine.h"
#include "Thread.h"
#include "Exe.h"

options c_options;
int startApplication(string e, List<native_string> &pArgs);

void init_main(List <native_string>& list1);

void createStringArray(Object *object, List<native_string> &lst);

void version() {
    cout << progname << " " << progvers << endl;
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

    if(!File::exists(executable.c_str())){
        error("file `" + executable + "` doesnt exist!");
    }

    return startApplication(executable, pArgs);
}

int startApplication(string exe, List<native_string>& pArgs) {
    int result;

    if(CreateVirtualMachine(exe) != 0) {
        fprintf(stderr, "Sharp VM init failed (check log file)\n");
        goto bail;
    }

    init_main(pArgs);
    vm->InterpreterThreadStart(Thread::threads.get(main_threadid));
    result=vm->exitVal;

    std::free(vm);
    std::free(env);
    return result;

    bail:
    if(vm != NULL) {
        vm->destroy();
    }

    return 1;
}
