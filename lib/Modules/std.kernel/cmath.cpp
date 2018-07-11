//
// Created by bknun on 7/11/2018.
//

#include <math.h>
#include "cmath.h"

double __cmath(double n, double n2, int proc) {

    switch(proc) {
        case 0x0:
            return sin(n);
        case 0x2:
            return cos(n);
        case 0x3:
            return tan(n);
        case 0x4:
            return sinh(n);
        case 0x5:
            return cosh(n);
        case 0x6:
            return tanh(n);
        case 0x7:
            return asin(n);
        case 0x8:
            return acos(n);
        case 0x9:
            return atan(n);
        case 0xa:
            return atan2(n, n2);
        case 0xb:
            return exp(n);
        case 0xc:
            return log(n);
        case 0xd:
            return log10(n);
        case 0xe:
            return pow(n, n2);
        case 0xf:
            return sqrt(n);
        case 0x10:
            return ceil(n);
        case 0x11:
            return floor(n);
        case 0x12:
            return fabs(n);
        case 0x13:
            return ldexp(n, (int)n2);
        case 0x14:
            return fmod(n, n2);
    }

    return -500;
}