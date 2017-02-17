//
// Created by BraxtonN on 2/17/2017.
//

#ifndef SHARP_ENVIRONMENT_H
#define SHARP_ENVIRONMENT_H


#include "../oo/Method.h"

class Environment {
public:

    Method* FindMethod(Method*);
    int CallMainMethod(Method*, void*, int);
    void DropLocals();

private:

};


#endif //SHARP_ENVIRONMENT_H
