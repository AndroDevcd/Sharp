//
// Created by bknun on 7/24/2018.
//

#include "Random.h"
#include "../../runtime/memory/garbage_collector.h"

#if _ARCH_BITS == 64
#include <random>
#endif

recursive_mutex rand_mut;
#if _ARCH_BITS == 64
std::default_random_engine generator;
std::uniform_int_distribution<Int> int_dist(0);
std::uniform_real_distribution<double> double_dist(0);
#endif

#if _ARCH_BITS == 32
double fRand(double fMin, double fMax)
{
    double f = (double)rand() / RAND_MAX;
    return fMin + f * (fMax - fMin);
}
#endif

void setSeed(unsigned int seed) {
    guard_mutex(rand_mut);
#if _ARCH_BITS == 64
    generator.seed(seed);
#else
    srand(seed);
#endif
}

Int randInt() {
    guard_mutex(rand_mut);
#if _ARCH_BITS == 64
    return int_dist(generator);
#else
    return rand();
#endif
}

Int randInt(Int min, Int max) {
    guard_mutex(rand_mut);

#if _ARCH_BITS == 64
    std::uniform_int_distribution<Int> ranged_int_dist(min, max);
    return ranged_int_dist(generator);
#else
    return min + rand() % (( max + 1 ) - min);
#endif
}

double randDouble() {
    guard_mutex(rand_mut);

#if _ARCH_BITS == 64
    return double_dist(generator);
#else
    return fRand(0, RAND_MAX);
#endif
}

double randDouble(double min, double max) {
    guard_mutex(rand_mut);

#if _ARCH_BITS == 64
    std::uniform_real_distribution<double> ranged_double_dist(min, max);
    return ranged_double_dist(generator);
#else
    return fRand(min, max);
#endif
}
