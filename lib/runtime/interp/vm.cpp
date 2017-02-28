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
#include "../oo/Reference.h"
#include "Opcode.h"

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
    env->nilReference = new Reference();

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

    updateStackFile("initializing memory objects");
    env->init();

    return 0;
}

void SharpVM::DestroySharpVM() {
    updateStackFile("Shutting down threads");
    if(Thread::self != NULL)
        Thread::self->exit();
    exitVal = Thread::self->exitVal;
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
                Thread::self->cstack.push(main);
                vm->Execute(main);
                Thread::self->cstack.pop();
            } else {
                // handle error
            }
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
    uint64_t *pc = &Thread::self->pc;
    Thread* self = Thread::self;

    int64_t address;
    *pc = method->entry;

    try {
        for (;;) {

            if(self->suspendPending)
                Thread::suspendSelf();
            if(self->state == thread_killed)
                return;

            switch((int)env->bytecode[(*pc)++]) {
                case nop:
                    break;
                case push_str:
                    self->stack.pushs(env->strings[(int64_t )env->bytecode[(*pc)++]].value);
                    break;
                case _int:
                    interrupt((int32_t )env->bytecode[(*pc)++]);
                    break;
                case pushi:
                    self->stack.push((int32_t )env->bytecode[(*pc)++]);
                    break;
                case ret:
                    return;
                case hlt:
                    self->state = thread_killed;
                    break;
                case _new:
                    address = (int64_t )env->bytecode[(*pc)++];
                    env->newClass(address, (int32_t )env->bytecode[(*pc)++]);
                    break;
                default:
                    // unsupported
                    break;
            }
        }
    } catch (Exception &e) {
        self->throwable = e.getThrowable();
        self->exceptionThrown = true;

        // TODO: handle exception
    }
}

void SharpVM::interrupt(int32_t signal) {
    switch (signal) {
        case 0x9f:
            cout << Thread::self->stack.popString();
            break;
    }
}
