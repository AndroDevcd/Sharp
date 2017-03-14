//
// Created by BraxtonN on 2/17/2017.
//

#ifndef SHARP_VM_H
#define SHARP_VM_H

#include "../../../stdimports.h"
#include "../oo/ClassObject.h"
#include "../oo/string.h"

class Environment;

class SharpVM {

public:
    void DestroySharpVM();

    static
#ifdef WIN32_
    DWORD WINAPI
#endif
#ifdef POSIX_
    void*
#endif
    InterpreterThreadStart(void *arg);

    void Shutdown();

    void interrupt(int32_t i);

    int exitVal;

    uint64_t Call(Method *pMethod);
};

extern SharpVM* vm;
extern Environment* env;

int CreateSharpVM(SharpVM**, Environment**, std::string, std::list<string>);

#endif //SHARP_VM_H
