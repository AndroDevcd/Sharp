//
// Created by bknun on 7/24/2018.
//

#ifndef SHARP_RANDOM_H
#define SHARP_RANDOM_H
#include "../../../stdimports.h"
#include "../../util/time.h"

void setSeed(unsigned int seed);

Int randInt();

Int randInt(Int min, Int max);

double randDouble();

double randDouble(double min, double max);

#endif //SHARP_RANDOM_H
