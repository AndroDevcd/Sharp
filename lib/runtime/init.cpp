//
// Created by BraxtonN on 2/10/2018.
//
#include <cstring>
#include <cstdio>
#include "../../stdimports.h"
#include "../util/File.h"
#include "oo/string.h"
#include "List.h"
#include "init.h"
#include "oo/Object.h"
#include "VirtualMachine.h"
#include "Thread.h"
#include "Exe.h"
#include "register.h"
#include "memory/GarbageCollector.h"
#include "Manifest.h"
#include "Environment.h"

options c_options;
int startApplication(string e, List<native_string> &pArgs);

void init_main(List <native_string>& list1);

void createStringArray(Object *object, List<native_string> &lst);

size_t getMemBytes(const char *argv, bool &setLimit);

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
    cout << "[-options]\n\n    -V                     print the bootstrap version number and exit" << endl;
    cout <<               "    -showversion           print the bootstrap version number and continue." << endl;
    cout <<               "    -Maxlmt<size:type>     set the maximum memory allowed to the virtual machine." << endl;
    cout <<               "    -gthreshold<size:type> set the minimum memory allowed to trigger the garbage collector." << endl;
    cout <<               "    --h -?                 display this help message." << endl;
}

#define opt(v) strcmp(argv[i], v) == 0

int runtimeStart(int argc, const char* argv[])
{
    if (argc < 2) { // We expect at least 1 argument: the executable
        help();
        return 1;
    }

    string executable ="";
    List<native_string> pArgs;

    /**
     * We start off with allowing 64 megabytes of memory to be under
     * mangement
     */
    GarbageCollector::initilize();
    GarbageCollector::setMemoryLimit(GC_HEAP_LIMIT);
    GarbageCollector::setMemoryThreshold(KB_TO_BYTES(64));

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
        else if(opt("-gthreashold") || opt("-gt")) {
            bool setLimit;
            GarbageCollector::setMemoryThreshold(getMemBytes(argv[++i], setLimit));

            if(!setLimit)
                error("expected postfix 'K', 'M', or 'G' after number with option `-gthreashold`");
        }
        else if(opt("-maxlmt")){
            if(i+1 >= argc)
                error("maximum memory limit required after option `-Maxlmt`");
            else {
                bool setLimit;
                GarbageCollector::setMemoryLimit(getMemBytes(argv[++i], setLimit));

                if(!setLimit)
                    error("expected postfix 'K', 'M', or 'G' after number with option `-Maxlmt`");
            }
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

size_t getMemBytes(const char *str, bool &setLimit) {
    setLimit= false;
    string size = string(str);
    stringstream ss;
    bool parsedDigit = false;

    for(unsigned int i = 0; i < size.size(); i++) {
        if(isdigit(size.at(i))) {
            parsedDigit = true;
            ss << size.at(i);
        } else if(isalpha(size.at(i))) {
            string num = ss.str();
            unsigned long limit = strtoul(ss.str().c_str(), NULL, 0);
            switch(size.at(i)) {
                case 'k':
                case 'K':
                    setLimit = true;
                    return KB_TO_BYTES(limit);
                case 'm':
                case 'M':
                    setLimit = true;
                    return MB_TO_BYTES(limit);
                case 'G':
                case 'g':
                    setLimit = true;
                    return GB_TO_BYTES(limit);
                default:
                    error("expected postfix 'K', 'M', or 'G' after number with option `-maxlmt`");
                    break;
            }
        } else {
            if(parsedDigit)
                error("expected postfix 'K', 'M', or 'G' after number with option `-maxlmt`");
            else
                error("expected number option `-maxlmt`");
        }
    }
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

void init_main(List <native_string>& pArgs) {
    Thread *main = Thread::threads.get(main_threadid);
    Object* object = &(++main->sp)->object;

    createStringArray(object, pArgs);
    for(unsigned int i = 0; i < pArgs.size(); i++) {
        pArgs.get(i).free();
    }
    pArgs.free();
}

void createStringArray(Object *object, List<native_string> &args) {
    int16_t MIN_ARGS = 4;
    int64_t size = MIN_ARGS+args.size();
    int64_t iter=0;

    stringstream ss;
    ss << manifest.target;
    native_string str(ss.str());

    object->object = GarbageCollector::self->newObjectArray(size);
    object->object->generation = gc_perm;

    GarbageCollector::self->createStringArray(&object->object->node[iter++], manifest.application);
    GarbageCollector::self->createStringArray(&object->object->node[iter++], manifest.version);
    GarbageCollector::self->createStringArray(&object->object->node[iter++], str); /* target platform also the platform version */

#ifdef WIN32_
    str = "win";
#endif
#ifdef POSIX_
    str = "posix";
#endif
    GarbageCollector::self->createStringArray(&object->object->node[iter++], str); /* operating system currently running on */

    /*
     * Assign program args to be passed to main
     */
    for(unsigned int i = 0; i < args.size(); i++) {
        GarbageCollector::self->createStringArray(&object->object->node[iter++], args.get(i));
    }
}

