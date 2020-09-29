//
// Created by bknun on 7/11/2018.
//

#include <math.h>
#include "cmath.h"
#include "../../runtime/symbols/Exception.h"
#include "../../runtime/VirtualMachine.h"

double __cmath(double n, double n2, int proc) {
    switch(proc) {
        case 0x0:
            return sin(n);
        case 0x1:
            return cos(n);
        case 0x2:
            return tan(n);
        case 0x3:
            return sinh(n);
        case 0x4:
            return cosh(n);
        case 0x5:
            return tanh(n);
        case 0x6:
            return asin(n);
        case 0x7:
            return acos(n);
        case 0x8:
            return atan(n);
        case 0x9:
            return atan2(n, n2);
        case 0xa:
            return exp(n);
        case 0xb:
            return log(n);
        case 0xc:
            return log10(n);
        case 0xd:
            return pow(n, n2);
        case 0xe:
            return sqrt(n);
        case 0xf:
            return ceil(n);
        case 0x10:
            return floor(n);
        case 0x11:
            return fabs(n);
        case 0x12:
            return ldexp(n, (int)n2);
        case 0x13:
            return fmod(n, n2);
        default: {
            stringstream ss;
            ss << "illegal call to math procedure: " << proc;
            throw Exception(vm.InvalidOperationExcept, ss.str());
        }
    }
}