#include <iostream>
#include "stdimports.h"
#include "lib/util/time.h"

Sharp versions;
#ifdef MAKE_COMPILER
    #include "lib/grammar/Runtime.h"
#endif
#ifndef MAKE_COMPILER
    #include "lib/runtime/init.h"

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
        cout << endl << "Compiled in " << ((now/1000)/1000) << "us & "
             << NANO_TOMILL(now-past) << "ms\n";

    cout << endl << "program exiting..." << endl;
    return 0;
}