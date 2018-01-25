#include <iostream>
#include "stdimports.h"
#include "lib/grammar/Runtime.h"
#include "lib/util/time.h"

int main(int argc, const char* argv[]) {
    uint64_t past= Clock::realTimeInNSecs(),now;
    _bootstrap( argc, argv );
    now= Clock::realTimeInNSecs();

    if(c_options.debugMode)
        cout << endl << "Compiled in " << NANO_TOMILL(now-past) << "ms\n";

    cout << "program exiting..." << endl;
    return 0;
}