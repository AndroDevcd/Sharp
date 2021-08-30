#include <iostream>
#include "stdimports.h"
#include "lib/util/time.h"

#ifdef MAKE_COMPILER
    #include "lib/grammar/main.h"
#endif
#ifndef MAKE_COMPILER
    #include "lib/runtime/main.h"
#include "lib/runtime/VirtualMachine.h"

#endif

int main(int argc, const char* argv[]) {
    uint64_t past= Clock::realTimeInNSecs(),now;
    Int result = 0;

#ifdef MAKE_COMPILER
    _bootstrap( argc, argv );
#endif
#ifndef MAKE_COMPILER
    result = runtimeStart( argc, argv );
#endif

    now= Clock::realTimeInNSecs();
    if(options.debug_mode)
        cout << endl << "Compiled in " << NANO_TOMICRO(now-past) << "us & "
             << NANO_TOMILL(now-past) << "ms\n";

    if(options.debug_mode)
        cout << endl << "program exiting..." << endl;
    return result;
}