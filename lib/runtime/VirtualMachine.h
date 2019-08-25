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

    static void sysInterrupt(int64_t i);

    //void executeMethod(int64_t address);
    int returnMethod();
    bool TryCatch(Method *method, Object *exceptionObject);
    void Throw();

    static void fillStackTrace(Object *exceptionObject);

    static void fillStackTrace(native_string &str);

    static string getPrettyErrorLine(long line, long sourceFile);

    int executeFinally(Method *method) {
        Thread *thread = thread_self;
        int64_t oldpc = PC(thread);

        for(long int i = 0; i < method->finallyBlocks.size(); i++) {
            FinallyTable &ft = method->finallyBlocks.get(i);
            if((ft.try_start_pc >= oldpc && ft.try_end_pc < oldpc) 
               || ft.start_pc > oldpc) {
                finallyTable = ft;
                startAddress = 1;
                thread->pc = thread->cache+ft.start_pc;

                /**
                 * Execute finally blocks before returning
                 */
                thread->exec();
                startAddress = 0;
            }
        }

        /**
         * If the finally block returns while we are trying to locate where the
         * exception will be caught we give up and the exception
         * is lost forever
         */
        return method == thread_self->current ? 0 : 1;
    }

    int exitVal;

    static void fillMethodCall(Frame &frame, stringstream &ss);

    static void __snprintf(int cfmt, double val, int precision);
};

extern VirtualMachine* vm;
extern Environment* env;
extern bool masterShutdown;

int CreateVirtualMachine(std::string);
void executeMethod(int64_t address, Thread* thread, bool inJit = false);
bool returnMethod(Thread* thread);
void invokeDelegate(int64_t address, int32_t args, Thread* thread, int64_t staticAddr);
int64_t executeSwitch(Thread* thread, int64_t constant);

#endif //SHARP_SHARP_H
