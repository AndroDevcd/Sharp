#include <iostream>
#include "stdimports.h"
#include "lib/util/time.h"

Sharp versions;
#ifdef MAKE_COMPILER
    #include "lib/grammar/Runtime.h"
#endif
#ifndef MAKE_COMPILER
    #include "lib/runtime/init.h"
#include "lib/runtime/VirtualMachine.h"
#include "lib/runtime/jit.h"

#endif

int main(int argc, const char* argv[]) {
    uint64_t past= Clock::realTimeInNSecs(),now;

#ifdef MAKE_COMPILER
    _bootstrap( argc, argv );
#endif
#ifndef MAKE_COMPILER
//    for(long i = 0; i < 100000000; i++) {
//        double a = 3;
//        double b = a + 9;
//        double c = a * b + (a * 1732737);
//    }
    runtimeStart( argc, argv );
#endif

    now= Clock::realTimeInNSecs();
    if(c_options.debugMode)
        cout << endl << "Compiled in " << NANO_TOMICRO(now-past) << "us & "
             << NANO_TOMILL(now-past) << "ms\n";

    cout << endl << "program exiting..." << endl;
    return 0;
}