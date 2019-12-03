//
// Created by braxtonn on 11/14/2019.
//

#include "Expression.h"
#include "Utype.h"

void Expression::init() {
    type = exp_undefined;
    utype = new Utype();
    lValue = false;
}

void Expression::free() {
    if(utype) {
        utype->free();
        delete utype;
    }
}
