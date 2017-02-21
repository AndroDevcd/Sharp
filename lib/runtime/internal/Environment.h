//
// Created by BraxtonN on 2/17/2017.
//

#ifndef SHARP_ENVIRONMENT_H
#define SHARP_ENVIRONMENT_H

#include "../oo/Method.h"

class gc_object;

class Environment {
public:

    int CallMainMethod(Method*, void*, int);
    void DropLocals();

    gc_object* objects;

};

extern Environment* env;

#define geto (offset) env->objects[env->]

#define mvers 1


#endif //SHARP_ENVIRONMENT_H
