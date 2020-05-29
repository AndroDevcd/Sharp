//
// Created by BraxtonN on 2/15/2018.
//

#ifndef SHARP_SHARP_H
#define SHARP_SHARP_H

#include "../../stdimports.h"
#include "symbols/ClassObject.h"
#include "symbols/string.h"
#include "Thread.h"
#include "symbols/Method.h"
#ifdef WIN32_
#include "../Modules/std.os.gui/win/Gui.h"
#endif
#include "Manifest.h"

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
    ClassObject* resolveClass(runtime::String fullName);
    Object* resolveField(runtime::String name, SharpObject *classObject);
    string stringValue(SharpObject *);
    bool isStaticObject(SharpObject *object);

    void shutdown();
    static void sysInterrupt(int64_t i);
    bool catchException();
    static void fillStackTrace(SharpObject *frameInfo, SharpObject *stackTrace);
    static void fillStackTrace(native_string &str);
    static void fillStackTrace(Object *methods, Object *pcList, Object *data);
    static string getPrettyErrorLine(long line, long sourceFile);
    static void getFrameInfo(Object *frameInfo);
    static void getStackTrace();


    static void fillMethodCall(Method* func, Int pc, stringstream &ss);
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
    ClassObject *InvalidOperationExcept;

    Object outOfMemoryExcept;
    Meta metaData;
    Manifest manifest;
    Object* staticHeap;
    ClassObject* classes;
    runtime::String* strings;
    double *constants;
    Method* methods;
    Symbol* nativeSymbols;
    Method* funcPtrSymbols;
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

#endif //SHARP_SHARP_H
