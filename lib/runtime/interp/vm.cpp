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

SharpVM* vm;
Environment* env;

int CreateSharpVM(std::string exe, std::list<string> pArgs)
{
    updateStackFile("Creating virtual machine:");
    vm = new SharpVM();
    env = new Environment();

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
    env->nilArray = new ArrayObject();
    env->emptyObject = new gc_object();

    /**
     * Aux classes
     */
    env->Throwable = new ClassObject(
            "sharp.lang#Throwable",
            new Field[0] { },
            0,
            new Method[0] {},
            0,
            NULL,
            ++manifest.baseaddr
    );

    env->RuntimeException = new ClassObject(
            "sharp.lang#RuntimeException",
            new Field[0] { },
            0,
            new Method[0] {},
            0,
            env->Throwable,
            ++manifest.baseaddr
    );

    env->StackOverflowErr = new ClassObject(
            "sharp.lang#StackOverflowErr",
            new Field[0] { },
            0,
            new Method[0] {},
            0,
            env->RuntimeException,
            ++manifest.baseaddr
    );

    env->ThreadStackException = new ClassObject(
            "sharp.lang#ThreadStackException",
            new Field[0] { },
            0,
            new Method[0] {},
            0,
            env->RuntimeException,
            ++manifest.baseaddr
    );

    env->IndexOutOfBoundsException = new ClassObject(
            "sharp.lang#IndexOutOfBoundsException",
            new Field[0] { },
            0,
            new Method[0] {},
            0,
            env->RuntimeException,
            ++manifest.baseaddr
    );

    env->NullptrException = new ClassObject(
            "sharp.lang#NullptrException",
            new Field[0] { },
            0,
            new Method[0] {},
            0,
            env->RuntimeException,
            ++manifest.baseaddr
    );

    updateStackFile("initializing memory objects");
    cout.precision(16);
    env->init(env->objects, manifest.classes);

    return 0;
}

void SharpVM::DestroySharpVM() {
    updateStackFile("Shutting down threads");
    if(Thread::self != NULL) {
        Thread::self->exit();
        exitVal = Thread::self->exitVal;
    } else
        exitVal = 1;
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
        Thread::self->state = thread_running;

        try {
            Method* main = Thread::self->main;
            if(main != NULL) {
                vm->Call(main);
            } else {
                // handle error
            }
        } catch (Exception &e) {
            Thread::self->throwable = e.getThrowable();
        }

    cout << "done..." << endl;
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
            cout << "done..." << endl;
            vm->Shutdown();
            cout << "done..." << endl;
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
    //env->shutdown();
}

void SharpVM::interrupt(int32_t signal) {
    switch (signal) {
        case 0x9f:
            cout << env->strings[(int64_t )Thread::self->stack.popn()].value;
            break;
        case 0xa0:
            Thread::self->stack.pop()->monitor->acquire();
            break;
        case 0xa1:
            Thread::self->stack.pop()->monitor->unlock();
            break;
        default:
            // unsupported
            break;
    }
}

uint64_t SharpVM::Call(Method *func) {
    //uint64_t pc = Thread::self->pc;
    Thread::self->cstack.push(func);
    Thread::self->cstack.instance = env->emptyObject;

    Thread::self->cstack.Execute();
    return 0;
}
