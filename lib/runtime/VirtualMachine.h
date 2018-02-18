//
// Created by BraxtonN on 2/15/2018.
//

#ifndef SHARP_SHARP_H
#define SHARP_SHARP_H

#include "../../stdimports.h"
#include "oo/ClassObject.h"
#include "oo/string.h"

class Environment;
class SharpObject;

class VirtualMachine {
public:
    void destroy();

    static
#ifdef WIN32_
    DWORD WINAPI
#endif
#ifdef POSIX_
    void*
#endif
    InterpreterThreadStart(void *arg);

    void shutdown();

    void sysInterrupt(int32_t i);

    void executeMethod(int64_t address);
    int returnMethod();
    bool TryThrow(Method* method, Object* exceptionObject);
    void Throw(Object *exceptionObject);

    void fillStackTrace(Object *exceptionObject);

    void fillStackTrace(native_string &str);

    string getPrettyErrorLine(long line, long sourceFile);

    int exitVal;
};

extern VirtualMachine* vm;
extern Environment* env;

int CreateVirtualMachine(std::string);

#endif //SHARP_SHARP_H
