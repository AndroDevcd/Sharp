//
// Created by BraxtonN on 2/15/2018.
//

#ifndef SHARP_SHARP_H
#define SHARP_SHARP_H

#include "../../stdimports.h"
#include "oo/ClassObject.h"
#include "oo/string.h"
#include "Thread.h"

class Environment;
struct SharpObject;

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

    //void executeMethod(int64_t address);
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
            if((ft.try_start_pc >= oldpc && ft.try_end_pc < oldpc) || ft.start_pc > oldpc) {
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

    void fillMethodCall(Frame frame, stringstream &ss, Frame *prev);
};

#define executeMethod(address) { \
 \
    Method *method = env->methods+address; \
 \
    if(thread_self->callStack.empty()) { \
        thread_self->callStack.add( \
                Frame(NULL, 0, 0, 0)); \
    } else { \
        thread_self->callStack.add( \
                Frame(thread_self->current, thread_self->pc, thread_self->sp-method->stackEqulizer, thread_self->fp)); \
    } \
     \
    thread_self->pc = 0; \
    thread_self->current = method; \
    thread_self->cache = method->bytecode; \
    thread_self->fp = thread_self->callStack.size()==1 ? thread_self->fp : \
                      ((method->returnVal) ? thread_self->sp-method->stackEqulizer : (thread_self->sp-method->stackEqulizer+1)); \
    thread_self->sp += (method->stackSize - method->paramSize); \
}

extern VirtualMachine* vm;
extern Environment* env;
extern bool masterShutdown;

int CreateVirtualMachine(std::string);

#endif //SHARP_SHARP_H
