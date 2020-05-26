//
// Created by braxtonn on 8/23/2019.
//

#include "Jit.h"
#include "../main.h"
#include "../Thread.h"
#include "../VirtualMachine.h"

#ifdef BUILD_JIT

Jit *Jit::self = nullptr;

void Jit::startup() { // TODO: update how instructions are processed because they have changed
    if(c_options.jit) {
        self = new Jit();

        if(self->getAssembler()) {
            if((self->error = self->getAssembler()->performInitialCompile()) != jit_error_ok) {
                switch(self->error) {
                    case jit_error_mem:
                        cout << "VM has low memory, consider adding more memory to the program to run the JIT (shutting down)." << endl;
                        Jit::shutdown();
                        return;
                    default:
                        /* do nothing */
                        break;
                }
            } else {
                auto* jitThread=(Thread*)malloc(
                        sizeof(Thread)*1);
                jitThread->CreateDaemon("jit");
                Thread::startDaemon(
                        Jit::threadStart, jitThread);
            }
        }
    }
}

void Jit::shutdown() {
    if(self) {
        if(self->x64Asm)
        {
            self->x64Asm->shutdown();
            delete self->x64Asm;
            self->x64Asm = NULL;
        }

        delete self; self = NULL;
    }
}

void Jit::run() {
    for(;;) {
        if(hasSignal(tSelf->signal, tsig_suspend))
            Thread::suspendSelf();
        if(tSelf->state == THREAD_KILLED) {
            return;
        }

        message:
        if(!messageQueue.empty()) {
            mutex.lock();
            int64_t function = messageQueue.last();
            messageQueue.pop_back();
            mutex.unlock();

            /**
             * We only want to run a concurrent compilation
             * of a function
             */
            getAssembler()->tryJit(vm.methods+function);
        }

        do {

#ifdef WIN32_
            Sleep(1);
#endif
#ifdef POSIX_
            usleep(1*999);
#endif
            if(!messageQueue.empty()) goto message;
        } while(!hasSignal(tSelf->signal, tsig_suspend) && tSelf->state == THREAD_RUNNING);
    }
}

#ifdef WIN32_
DWORD WINAPI
#endif
#ifdef POSIX_
void*
#endif
Jit::threadStart(void *pVoid) {
    thread_self =(Thread*)pVoid;
    thread_self->state = THREAD_RUNNING;
    self->tSelf = thread_self;

    try {
        self->run();
    } catch(Exception &e){
//        sendSignal(thread_self->signal, tsig_except, 1);
//        thread_self->throwable=e.getThrowable();
        // TODO: fix
    }

    /*
     * Check for uncaught exception in thread before exit
     */
    thread_self->exit();
#ifdef WIN32_
    return 0;
#endif
#ifdef POSIX_
    return nullptr;
#endif
}

void Jit::sendMessage(Method* func) {
    if(self && self->getAssembler()) {
        std::lock_guard<recursive_mutex> gd(self->mutex);
        func->compiling = true;
        self->messageQueue.push_back(func->address);
    }
}

_BaseAssembler *Jit::getAssembler() {
    //if(x64Asm) return x64Asm;
    return nullptr;
}

void Jit::tlsSetup() {
    Thread* thread = thread_self;
    thread->jctx->regs = registers;
    thread->jctx->self = thread;
}
#endif
