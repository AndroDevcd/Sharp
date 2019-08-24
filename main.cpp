#include <iostream>
#include "stdimports.h"
#include "lib/util/time.h"

Sharp versions;
#ifdef MAKE_COMPILER
    #include "lib/grammar/Runtime.h"
#endif
#ifndef MAKE_COMPILER
    #include "lib/runtime/main.h"
#include "lib/runtime/VirtualMachine.h"

#endif

int main(int argc, const char* argv[]) {
    uint64_t past= Clock::realTimeInNSecs(),now;

#ifdef MAKE_COMPILER
    _bootstrap( argc, argv );
#endif
#ifndef MAKE_COMPILER
    runtimeStart( argc, argv );
#endif

    now= Clock::realTimeInNSecs();
    if(c_options.debugMode)
        cout << endl << "Compiled in " << NANO_TOMICRO(now-past) << "us & "
             << NANO_TOMILL(now-past) << "ms\n";

    if(c_options.debugMode)
        cout << endl << "program exiting..." << endl;
    return 0;
}