//
// Created by BraxtonN on 2/15/2018.
//

#ifndef SHARP_SHARP_H
#define SHARP_SHARP_H

#include "../../stdimports.h"
#include "symbols/ClassObject.h"
#include "Thread.h"
#include "symbols/Method.h"
#ifdef WIN32_
#include "../Modules/std.os.gui/win/Gui.h"
#endif
#include "Manifest.h"
#include "Library.h"

struct SharpObject;

enum VMState {
    VM_CREATED = 0,
    VM_RUNNING = 1,
    VM_SHUTTING_DOWN = 2,
    VM_TERMINATED = 3
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
        ClassCastExcept(NULL),
        libs(),
        mutex(),
        libsMutex()
    {
    }
    recursive_mutex mutex;
    recursive_mutex libsMutex;
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
    ClassObject* resolveClass(std::string fullName);
    Object* resolveField(std::string name, SharpObject *classObject);
    void setFieldVar(std::string name, SharpObject *classObject, Int index, double value);
    void initializeField(std::string name, SharpObject *classObject, ClassObject* klass);
    string stringValue(SharpObject *);
    double numberValue(Int, SharpObject *);
    bool isStaticObject(SharpObject *object);

    void shutdown();
    static void sysInterrupt(int64_t i);
    static bool catchException();
    static void fillStackTrace(SharpObject *frameInfo, SharpObject *stackTrace);
    static void fillStackTrace(string &str);
    static void fillStackTrace(Object *methods, Object *pcList, Object *data);
    static string getPrettyErrorLine(long line, long sourceFile);
    static void getFrameInfo(Object *frameInfo);
    static void getStackTrace();
    void locateBridgeAndCross(Method*);
    bool link(string&, string&);



    static void fillMethodCall(Method* func, Int pc, stringstream &ss);
    static void __snprintf(int cfmt, double val, int precision);
    static bool isType(Object *obj, int32_t type);

    Library* getLib(std::string name);
    int freeLib(std::string name);

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
    ClassObject *UnsatisfiedLinkExcept;
    ClassObject *IncompatibleClassExcept;
    ClassObject *ObjectImportError;
    ClassObject *IllStateExcept;
    ClassObject *StringClass;
    ClassObject *StackSate;
    ClassObject *ThreadClass;
    ClassObject *ExceptionClass;
    ClassObject *ErrorClass;
    ClassObject *FiberClass;

    Object outOfMemoryExcept;
    Meta metaData;
    Manifest manifest;
    Object* staticHeap;
    ClassObject* classes;
    string* strings;
    double *constants;
    Method* methods;
    Symbol* nativeSymbols;
#ifdef WIN32_
    Gui* gui;
#endif
    _List<Library> libs;
    int exitVal;
    short state;
};

#define AUX_CLASSES 7

extern VirtualMachine vm;

int CreateVirtualMachine(string&);
void executeMethod(int64_t address, Thread* thread, bool inNativeEnv = false);
bool returnMethod(Thread* thread);
void invokeDelegate(int64_t address, int32_t args, Thread* thread, bool isStatic, bool inJit);
CXX11_INLINE
void setupMethodStack(int64_t address, Thread* thread, bool inJit);
extern void populateString(string&, double*, uInt);

#endif //SHARP_SHARP_H
