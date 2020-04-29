//
// Created by BraxtonN on 2/15/2018.
//

#ifndef SHARP_SHARP_H
#define SHARP_SHARP_H

#include "../../stdimports.h"
#include "oo/ClassObject.h"
#include "oo/string.h"
#include "Thread.h"
#include "oo/Method.h"
#ifdef WIN32_
#include "../Modules/std.os.gui/win/Gui.h"
#include "Manifest.h"

#endif

struct SharpObject;

enum VMState {
    VM_CREATED = 0,
    VM_RUNNING = 1,
    VM_TERMINATED = 2
};

class VirtualMachine {
public:
    VirtualMachine()
    :
        state(VM_CREATED),
        exitVal(0),
#ifdef WIN32_
        gui(NULL),
#endif
        methods(NULL),
        strings(NULL),
        classes(NULL),
        staticHeap(NULL),
        manifest(),
        metaData(),
        Throwable(NULL),
        StackOverflowExcept(NULL),
        RuntimeExcept(NULL),
        ThreadStackExcept(NULL),
        IndexOutOfBoundsExcept(NULL),
        NullptrExcept(NULL),
        ClassCastExcept(NULL)
    {
    }
    void destroy();

    static
#ifdef WIN32_
    DWORD WINAPI
#endif
#ifdef POSIX_
    void*
#endif
    InterpreterThreadStart(void *arg);
    Method* getMainMethod();
    ClassObject* resolveClass(String fullName);
    Object* resolveField(String name, Object *classObject);
    bool isStaticObject(Object *object);

    void shutdown();
    static void sysInterrupt(int64_t i);
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

    static void fillMethodCall(Method* func, Frame &frameInfo, stringstream &ss);
    static void __snprintf(int cfmt, double val, int precision);

    /**
     * Frequently used classes
     */
    ClassObject *Throwable;
    ClassObject *StackOverflowExcept;
    ClassObject *RuntimeExcept;
    ClassObject *ThreadStackExcept;
    ClassObject *IndexOutOfBoundsExcept;
    ClassObject *NullptrExcept;
    ClassObject *ClassCastExcept;
    ClassObject *OutOfMemoryExcept;

    Meta metaData;
    Manifest manifest;
    Object* staticHeap;
    ClassObject* classes;
    String* strings;
    double *constants;
    Method* methods;
#ifdef WIN32_
    Gui* gui;
#endif
    int exitVal;
    short state;
};

#define AUX_CLASSES 7

extern VirtualMachine vm;

int CreateVirtualMachine(string&);
fptr executeMethod(int64_t address, Thread* thread, bool inJit = false);
bool returnMethod(Thread* thread);
void invokeDelegate(int64_t address, int32_t args, Thread* thread, int64_t staticAddr);
int64_t executeSwitch(Thread* thread, int64_t constant);

#endif //SHARP_SHARP_H
