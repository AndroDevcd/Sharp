//
// Created by BraxtonN on 2/17/2017.
//

#ifndef SHARP_VM_H
#define SHARP_VM_H

#include "../../../stdimports.h"
#include "../oo/ClassObject.h"

class Environment;

class SharpVM {

public:
    void DestroySharpVM();

    void DetatchCurrentThread();

    Method* methods;
    ClassObject* classes;

};

extern SharpVM* vm;
extern Environment* env;

int CreateSharpVM(SharpVM**, Environment**, std::string, std::list<string>);

#endif //SHARP_VM_H
