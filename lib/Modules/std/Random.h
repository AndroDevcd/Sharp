//
// Created by bknun on 7/24/2018.
//

#ifndef SHARP_RANDOM_H
#define SHARP_RANDOM_H


#include <random>
#include "../../util/time.h"

typedef unsigned long int __seed_type;
/**
 * Thread safe random number generator
 */
class Random {
public:
    Random()
    :
        sd(0),
        generator(uniqueSeed()),
        dist(0,1),
        idist(0,1)
    {
    }

    double rand() {
        return dist(generator);
    }

    double randInt() {
        return idist(generator);
    }

    double rand(double min, double max) {
        decltype(dist.param()) new_range (min, max);
        dist.param(new_range);
        return dist(generator);
    }

    long long randInt(long long min, long long max) {
        decltype(idist.param()) new_range (min, max);
        idist.param(new_range);
        return idist(generator);
    }

    unsigned int reset() {
        generator.seed(uniqueSeed());
        return sd;
    }

    void seed(__seed_type sd) {
        generator.seed(sd);
    }

    __seed_type seed() {
        return sd;
    }

private:
    __seed_type sd;
    __seed_type uniqueSeed() {
        sd = (__seed_type)(NANO_TOMICRO(Clock::realTimeInNSecs())>>4);
        return sd;
    }
    std::mt19937 generator;
    std::uniform_real_distribution<double> dist;
    std::uniform_int_distribution<long long> idist;
};

double __crand(int proc);

#endif //SHARP_RANDOM_H
