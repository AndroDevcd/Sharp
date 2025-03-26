#include <iostream>
#include "stdimports.h"
#include "lib/util/time.h"

#ifdef MAKE_COMPILER
    #include "lib/grammar/main.h"
#endif
#ifndef MAKE_COMPILER
    #include "lib/runtime/main.h"
#endif

 Int serializeTime = 0;

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

#ifdef VM_DEBUG
    cout << endl << "Serialized in " << serializeTime << "us\n";
#endif
    cout << endl << "Executed in " << NANO_TOMICRO(now-past) << "us & "
             << NANO_TOMILL(now-past) << "ms\n";
    cout << endl << "vm exiting..." << endl;
    return result;
}