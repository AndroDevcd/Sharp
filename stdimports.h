//
// Created by bknun on 1/6/2017.
//

#ifndef SHARP_STDIMPORTS_H
#define SHARP_STDIMPORTS_H

#include <iostream>
#include <stdint.h>
#include <cstdlib>
#include <sstream>
#include <list>

using namespace std;

#define WIN32_
//#define POSIX_

#ifdef WIN32_
    #ifndef __wtypes_h__
    #include <wtypes.h>
    #endif

    #ifndef __WINDEF_
    #include <windef.h>
    #endif
#endif
#ifdef POSIX_
    #include <pthread.h>
#endif

#define CXX11_INLINE inline

#endif //SHARP_STDIMPORTS_H
