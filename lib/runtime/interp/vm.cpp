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
    vm = (SharpVM*)malloc(sizeof(SharpVM)*1);
    env = (Environment*)malloc(sizeof(Environment)*1);

    if(Process_Exe(exe) != 0)
        return 1;

    Thread::Startup();

    /**
     * Aux classes
     */
    env->Throwable = ClassObject(
            "sharp.lang#Throwable",
            NULL,
            0,
            NULL,
            0,
            NULL,
            ++manifest.baseaddr
    );

    env->RuntimeException = ClassObject(
            "sharp.lang#RuntimeException",
            NULL,
            0,
            NULL,
            0,
            &env->Throwable,
            ++manifest.baseaddr
    );

    env->StackOverflowErr = ClassObject(
            "sharp.lang#StackOverflowErr",
            NULL,
            0,
            NULL,
            0,
            &env->RuntimeException,
            ++manifest.baseaddr
    );

    env->ThreadStackException = ClassObject(
            "sharp.lang#ThreadStackException",
            NULL,
            0,
            NULL,
            0,
            &env->RuntimeException,
            ++manifest.baseaddr
    );

    env->IndexOutOfBoundsException = ClassObject(
            "sharp.lang#IndexOutOfBoundsException",
            NULL,
            0,
            NULL,
            0,
            &env->RuntimeException,
            ++manifest.baseaddr
    );

    env->NullptrException = ClassObject(
            "sharp.lang#NullptrException",
            NULL,
            0,
            NULL,
            0,
            &env->RuntimeException,
            ++manifest.baseaddr
    );
    cout.precision(16);
    env->init(env->objects, manifest.classes);

    return 0;
}

void SharpVM::DestroySharpVM() {
    cout << "natively exiting thread" << endl;
    Thread* t = thread_self;
    if(t != NULL) {
        t->exit();
        exitVal = t->exitVal;
    } else
        exitVal = 1;
    cout << "shutting down thread system" << endl;
    Thread::shutdown();
}

#ifdef WIN32_
DWORD WINAPI
#endif
#ifdef POSIX_
void*
#endif
    SharpVM::InterpreterThreadStart(void *arg) {
        Thread* self = (Thread*)arg;
        thread_self = self;
        self->state = thread_running;

        try {
            Method* main = self->main;
            if(main != NULL) {
                cout << "calling main" << endl;
                vm->Call(main);
            } else {
                // handle error
            }
        } catch (Exception &e) {
            self->throwable = e.getThrowable();
        }

    cout << "main ended" << endl;
        if (self->id == main_threadid)
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
            self->exit();
        }

#ifdef WIN32_
        return 0;
#endif
#ifdef POSIX_
        return NULL;
#endif
    }

void SharpVM::Shutdown() {
    cout << "destroying vm" << endl;
    DestroySharpVM();
    cout << "shutting down memory" << endl;
    env->shutdown();
}

void SharpVM::interrupt(int32_t signal) {
    switch (signal) {
        case 0x9f:
            cout << env->strings[(int64_t )thread_self->stack.popn()].value.str();
            break;
        case 0xa0: // TodO: convert to instructions
            thread_self->stack.pop()->monitor.acquire();
            break;
        case 0xa1:
            thread_self->stack.pop()->monitor.unlock();
            break;
        default:
            // unsupported
            break;
    }
}

uint64_t SharpVM::Call(Method *func) {
    //uint64_t pc = thread_self->pc;
    thread_self->cstack.push(func);
    thread_self->cstack.instance = NULL;

    thread_self->cstack.Execute();
    return 0;
}
