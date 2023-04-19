#include <iostream>
#include "stdimports.h"
#include "lib/util/time.h"

#ifdef MAKE_COMPILER
    #include "lib/grammar/main.h"
    #include "lib/old_grammar/options.h"
#endif
#ifndef MAKE_COMPILER
    #include "lib/runtime/main.h"
#endif

int main(int argc, const char* argv[]) {
    uint64_t past= Clock::realTimeInNSecs(),now;
    Int result = 0;

#ifdef MAKE_COMPILER
    _bootstrap( argc, argv );
#endif
#ifndef MAKE_COMPILER
    result = str_start( argc, argv );
#endif

    now= Clock::realTimeInNSecs();

    cout << endl << "Compiled in " << NANO_TOMICRO(now-past) << "us & "
             << NANO_TOMILL(now-past) << "ms\n";
    cout << endl << "program exiting..." << endl;
    return result;
}