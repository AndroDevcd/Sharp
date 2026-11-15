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
    // A program's output ends with the program's last line: the VM and the compiler add nothing of
    // their own. (Until 4.0.0r10 every run ended with "Executed in ..us & ..ms" and "vm exiting...".
    // Build with -DSHARP_SHOW_RUN_TIME to get the two lines back, e.g. for timing by hand.)
#ifdef SHARP_SHOW_RUN_TIME
    cout << endl << "Executed in " << NANO_TOMICRO(now-past) << "us & "
             << NANO_TOMILL(now-past) << "ms\n";
    cout << endl << "vm exiting..." << endl;
#endif
    return result;
}