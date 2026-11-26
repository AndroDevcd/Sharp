//
// Created by bknun on 7/11/2018.
//

#ifndef SHARP_MATH_H
#define SHARP_MATH_H

#include "../../stdimports.h"

double __cmath(double n, double n2, int proc);

/** the same for an instruction, which cannot raise: a number that is no function gives "not a number" */
double math_function(int proc, double n, double n2);

#endif //SHARP_MATCH_H
