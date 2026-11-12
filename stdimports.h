//
// Created by bknun on 1/6/2017.
//

#ifndef SHARP_STDIMPORTS_H
#define SHARP_STDIMPORTS_H

#include <iostream>
#include <cstdlib>
#include <sstream>
#include <list>
#include <stdexcept>
#include <string>
#include <cstdio>
#include <cstring>
#include <thread>
#include <limits>
#include <cmath>
#include <mutex>
#include <atomic>
#include <memory>
#include "lib/core/architecture.h"
#include "lib/grammar/platform.h"

using namespace std;

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
#define WIN32_
#else
#define POSIX_
#endif

#ifdef WIN32_

#define __os_yield() std::this_thread::yield();
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#ifndef __WINDEF_
#include <windef.h>
#endif
#endif
#ifdef POSIX_
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdarg.h>
#include <dlfcn.h>

#define POSIX_USEC_INTERVAL 1000

#define MUTEX pthread_mutex_t
#define __os_yield() sched_yield();
#endif

//#define SAFE_EXECUTION
//#define VM_DEBUG
// #define SHARP_PROF

// #define COROUTINE_DEBUGGING

// Built-in VM performance telemetry (lib/runtime/telemetry). When defined the
// runtime records where time is spent (interpreter vs jit vs gc vs waiting),
// context switches, hot opcodes/functions, why functions could not be jit
// compiled, etc. and writes sharp_telemetry.txt when the program exits.
// When NOT defined none of that code is compiled in (zero cost).
// #define SHARP_TELEMETRY

// Built-in VM performance telemetry (lib/runtime/telemetry). When defined the
// runtime records where time is spent (interpreter vs jit vs gc vs waiting),
// context switches, hot opcodes/functions, why functions could not be jit
// compiled, etc. and writes sharp_telemetry.txt when the program exits.
// When NOT defined none of that code is compiled in (zero cost).
// #define SHARP_TELEMETRY

#ifdef SHARP_PROF_
#define PROFILER_NAME "tanto"
#endif

#ifndef DEBUGGING
#define DEBUGGING
#endif

#if _ARCH_BITS == 32
typedef int32_t Int;
typedef uint32_t uInt;
#define _INT_MAX INT32_MAX
#else
typedef int64_t Int;
typedef uint64_t uInt;
#define _INT_MAX INT64_MAX
#endif

/**
 * A number as the virtual machine stores it: in a register, in a stack slot, in
 * a number array or a numeric field.
 *
 * It is 64 bits that are EITHER an integer OR a decimal. Nothing in the cell
 * says which: the compiler knows the kind of every value and emits the
 * instruction that reads the right member (ADD reads .i, DADD reads .d, see
 * lib/core/opcode/opcode.h). Moving a number copies the whole cell and never
 * looks inside.
 *
 *      C++ comparison: a union with no tag. Reading the member that was not
 *      written is exactly the mistake the kind checker
 *      (runtime/memory/type_check.h) exists to find.
 *
 * There is deliberately no arithmetic on the cell itself: code that wants a
 * number has to say .i or .d, so the C++ compiler points at every place that
 * has not decided.
 *
 * History: every number used to be a `long double` (80-bit x87, 16 bytes),
 * then a double; whole numbers were only exact up to 2^53.
 */
union vm_cell {
    int64_t i;      // an integer: wraps around on overflow
    double d;       // a decimal
};

/**
 * decimal -> integer, the conversion the D2I instruction does: the fraction is
 * dropped (2.9 -> 2, -2.9 -> -2). A plain C++ cast is undefined for values
 * that do not fit and for NaN, so those are given fixed results here:
 * NaN -> 0, too large -> the largest integer, too small -> the smallest.
 */
inline int64_t decimal_to_integer(double value) {
    if(value != value) return 0;
    if(value >= 9223372036854775807.0) return INT64_MAX;
    if(value <= -9223372036854775808.0) return INT64_MIN;
    return (int64_t)value;
}

#define CXX11_INLINE inline


#endif //SHARP_STDIMPORTS_H
