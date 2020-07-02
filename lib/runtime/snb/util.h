//
// Created by BNunnally on 6/27/2020.
//

#ifndef SHARP_UTIL_H
#define SHARP_UTIL_H

// Define EXPORTED for any platform
#if defined _WIN32 || defined __CYGWIN__
#define EXPORTED __attribute__ ((dllexport))
#define NOT_EXPORTED
#else
#if __GNUC__ >= 4
    #define EXPORTED __attribute__ ((visibility ("default")))
    #define NOT_EXPORTED  __attribute__ ((visibility ("hidden")))
  #else
    #define EXPORTED
    #define NOT_EXPORTED
  #endif
#endif

typedef void* object;

#endif //SHARP_UTIL_H
