#include <iostream>
#include "stdimports.h"
#include "lib/util/time.h"

Sharp versions;
#ifdef MAKE_COMPILER
    #include "lib/grammar/Runtime.h"
#endif
#ifndef MAKE_COMPILER
    #include "lib/runtime/init.h"
    #define WIN32_LEAN_AND_MEAN

#endif

int locked = 0;
int unlocked = 0;

void test() {

    std::list<int> lst;
    std::list<int>::iterator it;

    lst.push_back(1);
    lst.push_back(2);
    lst.push_back(3);
    lst.push_back(4);

    int *a = &lst.back(), *b;

    for (it=lst.begin(); it!=lst.end(); ++it){
        if(*it == 2) {
            lst.erase(it);
            break;
        }
    }

    b = &lst.back();

    cout << " a " << a << " b " << b << endl;
    lst.clear();
}

uint64_t past,now;

int main(int argc, const char* argv[]) {
    past= Clock::realTimeInNSecs();

    //test();
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

    cout << "lock() called " << locked << endl;
    cout << endl << "program exiting..." << endl;
    return 0;
}