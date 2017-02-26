//
// Created by BraxtonN on 2/17/2017.
//

#include "../internal/Environment.h"
#include "vm.h"
#include "../internal/Exe.h"
#include "../internal/Thread.h"
#include "../oo/Field.h"
#include "../oo/Method.h"
#include "../oo/Array.h"
#include "../oo/Object.h"
#include "../oo/Exception.h"

SharpVM* vm;
Environment* env;

int CreateSharpVM(SharpVM** pVM, Environment** pEnv, std::string exe, std::list<string> pArgs)
{
    updateStackFile("Creating virtual machine:");
    vm = new SharpVM();
    env = new Environment();

    *pVM = vm;
    *pEnv = env;

    if(Process_Exe(exe) != 0)
        return 1;

    Thread::Startup();

    updateStackFile("(internal) Adding helper classes and objects");

    env->nilClass = new ClassObject(
            "$internal#NillClass",
            new Field[0] {},
            0,
            new Method[0] {},
            0,
            NULL,
            ++manifest.baseaddr
    );

    /**
     * Native objects cannot be null but this will be used
     * for assigning objects to prevent seg faults on invalid
     * instruction executions
     */
    env->nilObject = new Object();
    env->nilArray = new ArrayObject();

    return 0;
}

void SharpVM::DestroySharpVM() {
    updateStackFile("Shutting down threads");
    Thread::self->exit();
    Thread::shutdown();
}

#ifdef WIN32_
DWORD WINAPI
#endif
#ifdef POSIX_
void*
#endif
    SharpVM::InterpreterThreadStart(void *arg) {
        Thread::self = (Thread*)arg;

        try {
            Method* main = Thread::self->main;
            vm->Execute(main);
        } catch (Exception &e) {
            Thread::self->throwable = e.getThrowable();
        }

        if (Thread::self->id == main_threadid)
        {
            /*
            * Shutdown all running threads
            * and de-allocate al allocated
            * memory. If we do not call join()
            * to wait for all other threads
            * regardless of what they are doing, we
            * stop them.
            */
            vm->Shutdown();
        }
        else
        {
            /*
             * Check for uncaught exception in thread before exit
             */
            Thread::self->exit();
        }

#ifdef WIN32_
        return 0;
#endif
#ifdef POSIX_
        return NULL;
#endif
    }

void SharpVM::Shutdown() {
    DestroySharpVM();
    env->shutdown();
}

void SharpVM::Execute(Method *method) {
    // TODO: write bytecode interpreter
}
