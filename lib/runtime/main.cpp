//
// Created by bknun on 9/16/2022.
//

#include "main.h"
#include "../core/exe_macros.h"
#include "memory/garbage_collector.h"
#include "multitasking/thread/stack_limits.h"
#include "memory/vm_stack.h"
#include "../util/File.h"
#include "vm_initializer.h"
#include "virtual_machine.h"

string executable;

void help() {
#ifndef SHARP_PROF_
    std::cerr << "Usage: " << progname << " {OPTIONS} EXECUTABLE" << std::endl;
#endif
#ifdef SHARP_PROF_
    std::cerr << "Usage: " << PROFILER_NAME << "  {OPTIONS} EXECUTABLE" << std::endl;
#endif
    cout << "Executable must be built with sharpc to be executed\n" << endl;
    cout << "[-options]\n\n    -V                     print the version number and exit" << endl;
    cout <<               "    -showversion           print the version number and continue." << endl;
    cout <<               "    -mem<size:type>        set the maximum memory allowed to the virtual machine." << endl;
    cout <<               "    -stack<size:type>      set the default physical stack size allowed to threads." << endl;
    cout <<               "    -istack<size:type>     set the default internal stack size allotted to the virtual machine." << endl;
    cout <<               "    --h -?                 display this help message." << endl;
}

void print_version() {
    cout << progname << " " << progvers << " build-" << BUILD_VERS << endl;
}

void error(string message) {
    cout << "sharp:  error: " << message << endl;
    exit(1);
}

uInt mem_str_to_bytes(const char *str, string option) {
    string size = string(str);
    stringstream ss;
    bool parsedDigit = false;

    for(unsigned int i = 0; i < size.size(); i++) {
        if(isdigit(size.at(i))) {
            parsedDigit = true;
            ss << size.at(i);
        } else if(isalpha(size.at(i))) {
            string num = ss.str(); // 1028M
            unsigned long limit = strtoul(ss.str().c_str(), NULL, 0);
            switch(size.at(i)) {
                case 'k':
                case 'K':
                    return KB_TO_BYTES(limit);
                case 'm':
                case 'M':
                    return MB_TO_BYTES(limit);
                case 'G':
                case 'g':
                    return GB_TO_BYTES(limit);
                default:
                    error("expected postfix 'K', 'M', or 'G' after number with option `" + option + "`");
                    break;
            }
        } else {
            if(parsedDigit)
                error("expected postfix 'K', 'M', or 'G' after number with option `" + option + "`");
            else
                error("expected number option `" + option + "`");
        }
    }
    return 0;
}

int run_app() {
    int result;
    if ((result = initialize_virtual_machine()) != 0) {
        fprintf(stderr, "Could not start the Sharp virtual machine. Failed with code: %d\n", result);
        goto bail;
    }

    if((vm.manifest.threadLocals + (sizeof(stack_item) / KB_TO_BYTES(25))) >= virtualStackSize) {
        unsigned int oldSize = (virtualStackSize * sizeof(stack_item)) / KB_TO_BYTES(1);
        virtualStackSize = vm.manifest.threadLocals + (KB_TO_BYTES(64) / sizeof(stack_item));
        unsigned int updatedBytes = (virtualStackSize * sizeof(stack_item)) / KB_TO_BYTES(1);
        if(internalStackUpdated) {
            fprintf(stdout, "Virtual stack size of: %u Kib was too small for the application \"%s\" updating stack size to %u Kib\n",
                    oldSize, vm.manifest.application.c_str(), updatedBytes);
        }
    }

    bail:
    shutdown();
    return result;
}

#define opt(v) strcmp(argv[i], v) == 0
int  str_start(int argc, const char* argv[]) {
    if (argc < 2) { // We expect at least 1 argument: the executable
        help();
        return 1;
    }

    // todo initialize garbage collector here

    for (int i = 1; i < argc; ++i) {
        if(opt("-V")){
            print_version();
            exit(0);
        }
        else if(opt("-h") || opt("-?")){
            help();
            exit(0);
        }
        else if(opt("-showversion")){
            print_version();
        }
        else if(opt("-debug")) {
            c_options.debugMode = true;
        }
#ifdef SHARP_PROF_
            else if(opt("-sort") || opt("-sortby")) {
            if((i+1) >= argc)
                error("expected argument after option `-sort`");

            i++;
            if(opt("tm")) {
                c_options.sortBy = profilerSort::tm;
            } else if(opt("avgt") || opt("avgtm")) {
                c_options.sortBy = avgt;
            } else if(opt("calls")) {
                c_options.sortBy = calls;
            } else if(opt("ir")) {
                c_options.sortBy = ir;
            } else {
                error("invalid argument after option `-sort`");
            }
        }
#endif
        else if(opt("-maxlmt") || opt("-mem")){
            if(i+1 >= argc)
                error("maximum memory limit required after option `" + string(argv[i]) + "`");
            else {
                // todo: set garbage collector memory limit
                i++;
            }
        }
        else if(opt("-stack")){
            if(i+1 >= argc)
                error("maximum stack limit required after option `-stack`");
            else {
                size_t sz = mem_str_to_bytes(argv[i+1], argv[i]);
                i++;

                if(valid_stack_size(sz)) {
                    threadStackSize = sz;
                } else {
                    stringstream ss;
                    ss << "default stack size must be greater than " << STACK_MIN << " bytes \n";
                    error(ss.str());
                }
            }
        }
        else if(opt("-istack")){
            if(i+1 >= argc)
                error("internal stack limit required after option `-istack`");
            else {
                size_t memoryLimit = mem_str_to_bytes(argv[i+1], argv[i]);
                size_t stackSize = memoryLimit / sizeof(stack_item);
                i++;

                if(valid_internal_stack_size(memoryLimit)) {
                    virtualStackSize = stackSize;
                } else {
                    stringstream ss;
                    ss << "default internal stack size must be at least " << INTERNAL_STACK_MIN << " bytes \n";
                    error(ss.str());
                }
            }
        }
        else if(string(argv[i]).at(0) == '-'){
            error("invalid option `" + string(argv[i]) + "`, try sharp -h");
        }
        else {
            executable = argv[i++];
            while(i < argc) {
                exeArgs.emplace_back(argv[i++]);
            }
            break;
        }
    }

    if(executable.empty()){
        help();
        return 1;
    }

    if(!File::exists(executable.c_str())){
        error("file `" + executable + "` doesnt exist!");
    }

    return run_app();
}
