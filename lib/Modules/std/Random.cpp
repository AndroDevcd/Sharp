//
// Created by bknun on 7/24/2018.
//

#include <random>
#include "Random.h"
#include "../../runtime/Thread.h"

std::default_random_engine generator;
std::recursive_mutex rand_mut;
std::uniform_int_distribution<Int> int_dist(0);
std::uniform_real_distribution<double> double_dist(0);

void setSeed(unsigned int seed) {
    GUARD(rand_mut);
    generator.seed(seed);
}

Int randInt() {
    GUARD(rand_mut);
    return int_dist(generator);
}

Int randInt(Int min, Int max) {
    GUARD(rand_mut);
    std::uniform_int_distribution<Int> ranged_int_dist(min, max);
    return ranged_int_dist(generator);
}

double randDouble() {
    GUARD(rand_mut);
    return double_dist(generator);
}

double randDouble(double min, double max) {
    GUARD(rand_mut);
    std::uniform_real_distribution<double> ranged_double_dist(min, max);
    return ranged_double_dist(generator);
}
