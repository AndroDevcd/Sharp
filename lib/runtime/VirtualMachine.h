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

    CXX11_INLINE
    void executeFinally(Method *method) {
        uint64_t oldpc = thread_self->pc;

        for(unsigned int i = 0; i < method->finallyBlocks.size(); i++) {
            FinallyTable &ft = method->finallyBlocks.get(i);
            if(ft.try_start_pc >= oldpc && ft.try_end_pc < oldpc) {
                finallyTable = ft;
                startAddress = 1;
                thread_self->pc = ft.start_pc;

                /**
                 * Execute finally blocks before returning
                 */
                thread_self->exec();
                startAddress = 0;
            }
        }
    }

    int exitVal;
};

extern VirtualMachine* vm;
extern Environment* env;

int CreateVirtualMachine(std::string);

#endif //SHARP_SHARP_H
