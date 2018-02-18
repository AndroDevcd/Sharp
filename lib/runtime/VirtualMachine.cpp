//
// Created by BraxtonN on 2/15/2018.
//

#include "VirtualMachine.h"
#include "Exe.h"
#include "Thread.h"
#include "memory/GarbageCollector.h"
#include "register.h"
#include "Environment.h"
#include "../util/time.h"
#include "Opcode.h"

VirtualMachine* vm;
Environment* env;

int CreateVirtualMachine(std::string exe)
{
    vm = (VirtualMachine*)__malloc(sizeof(VirtualMachine)*1);
    env = (Environment*)__malloc(sizeof(Environment)*1);

    if(Process_Exe(exe) != 0)
        return 1;

    Thread::Startup();
    GarbageCollector::startup();

    /**
     * Aux classes
     */
    Field* fields=(Field*)malloc(sizeof(Field)*2);
    fields[0].init("message", 0, VAR, false, false, &env->classes[manifest.classes]);
    fields[1].init("stackTrace", 0, VAR, false, false, &env->classes[manifest.classes]);

    env->classes[manifest.classes].init();
    env->classes[manifest.classes] = ClassObject(
            "std.err#Throwable",
            fields,
            2,
            NULL,
            manifest.classes
    );
    env->Throwable = &env->classes[manifest.classes++];

    fields=(Field*)malloc(sizeof(Field)*2);
    fields[0].init("message", 0, VAR, false, false, env->Throwable);
    fields[1].init("stackTrace", 0, VAR, false, false, env->Throwable);

    env->classes[manifest.classes].init();
    env->classes[manifest.classes] = ClassObject(
            "std.err#RuntimeErr",
            fields,
            2,
            env->Throwable,
            manifest.classes
    );
    env->RuntimeErr = &env->classes[manifest.classes++];

    fields=(Field*)malloc(sizeof(Field)*2);
    fields[0].init("message", 0, VAR, false, false, env->Throwable);
    fields[1].init("stackTrace", 0, VAR, false, false, env->Throwable);

    env->classes[manifest.classes].init();
    env->classes[manifest.classes] = ClassObject(
            "std.err#StackOverflowErr",
            fields,
            2,
            env->RuntimeErr,
            manifest.classes
    );
    env->StackOverflowErr = &env->classes[manifest.classes++];

    fields=(Field*)malloc(sizeof(Field)*2);
    fields[0].init("message", 0, VAR, false, false, env->Throwable);
    fields[1].init("stackTrace", 0, VAR, false, false, env->Throwable);

    env->classes[manifest.classes].init();
    env->classes[manifest.classes] = ClassObject(
            "std.err#ThreadStackException",
            fields,
            2,
            env->RuntimeErr,
            manifest.classes
    );
    env->ThreadStackException = &env->classes[manifest.classes++];

    fields=(Field*)malloc(sizeof(Field)*2);
    fields[0].init("message", 0, VAR, false, false, env->Throwable);
    fields[1].init("stackTrace", 0, VAR, false, false, env->Throwable);

    env->classes[manifest.classes].init();
    env->classes[manifest.classes] = ClassObject(
            "std.err#IndexOutOfBoundsException",
            fields,
            2,
            env->RuntimeErr,
            manifest.classes
    );
    env->IndexOutOfBoundsException = &env->classes[manifest.classes++];

    fields=(Field*)malloc(sizeof(Field)*2);
    fields[0].init("message", 0, VAR, false, false, env->Throwable);
    fields[1].init("stackTrace", 0, VAR, false, false, env->Throwable);

    env->classes[manifest.classes].init();
    env->classes[manifest.classes] = ClassObject(
            "std.err#NullptrException",
            fields,
            2,
            env->RuntimeErr,
            manifest.classes
    );
    env->NullptrException = &env->classes[manifest.classes++];

    fields=(Field*)malloc(sizeof(Field)*2);
    fields[0].init("message", 0, VAR, false, false, env->Throwable);
    fields[1].init("stackTrace", 0, VAR, false, false, env->Throwable);

    env->classes[manifest.classes].init();
    env->classes[manifest.classes] = ClassObject(
            "std.err#ClassCastException",
            fields,
            2,
            env->RuntimeErr,
            manifest.classes
    );
    env->ClassCastException = &env->classes[manifest.classes++];

    cout.precision(16);

    for(unsigned long i = 0; i < manifest.classes; i++) {
        env->globalHeap[i].object = NULL;
    }

    return 0;
}

void VirtualMachine::destroy() {
    if(thread_self != NULL) {
        exitVal = thread_self->exitVal;
    } else
        exitVal = 1;
    Thread::shutdown();
    GarbageCollector::self->shutdown();
}

#ifdef WIN32_
DWORD WINAPI
#endif
#ifdef POSIX_
void*
#endif
VirtualMachine::InterpreterThreadStart(void *arg) {
    thread_self = (Thread*)arg;
    thread_self->state = THREAD_RUNNING;

    try {
        thread_self->exec();
    } catch (Exception &e) {
        //    if(thread_self->exceptionThrown) {
        //        cout << thread_self->throwable.stackTrace.str();
        //    }
        thread_self->throwable = e.getThrowable();
        thread_self->exceptionThrown = true;
        cout << "Uncaught Exception: " << e.throwable.throwable->name.str()
             << ": " << e.throwable.message.str();
    }

    /*
     * Check for uncaught exception in thread before exit
     */
    thread_self->exit();

    if (thread_self->id == main_threadid)
    {
        /*
        * Shutdown all running threads
        * and de-allocate all allocated
        * memory. If we do not call join()
        * to wait for all other threads
        * regardless of what they are doing, we
        * stop them.
        */
        vm->shutdown();
    }

#ifdef WIN32_
    return 0;
#endif
#ifdef POSIX_
    return NULL;
#endif
}

void VirtualMachine::shutdown() {
    destroy();
    env->shutdown();
}

void VirtualMachine::sysInterrupt(int32_t signal) {
    switch (signal) {
        case 0x9f:
            //cout << env->strings[(int64_t )thread_self->__stack[(int64_t)__rxs[sp]--].var].value.str();
            return;
        case 0xa0:
            registers[bmr]= Clock::__os_time((int) registers[ebx]);
            return;
        case 0xa1:
            /**
             * Make an explicit call to the garbage collector. This
             * does not garuntee that it will run
             */
            GarbageCollector::self->sendMessage(
                    CollectionPolicy::GC_EXPLICIT);
            return;
        case 0xa2:
            /**
             * This should only be used in low memory conditions
             * Sending this request will freeze your entire application
             * for an unspecified amount of time
             */
            GarbageCollector::self->sendMessage(
                    CollectionPolicy::GC_LOW);
            return;
        case 0xa3:
            registers[bmr]= Clock::realTimeInNSecs();
            return;
        case 0xa4:
            registers[cmt]=Thread::start((int32_t )registers[adx]);
            return;
        case 0xa5:
            registers[cmt]=Thread::join((int32_t )registers[adx]);
            return;
        case 0xa6:
            registers[cmt]=Thread::interrupt((int32_t )registers[adx]);
            return;
        case 0xa7:
            registers[cmt]=Thread::destroy((int32_t )registers[adx]);
            return;
        case 0xa8:
            registers[cmt]=Thread::Create((int32_t )registers[adx], (unsigned long)registers[egx]);
            return;
        default:
            // unsupported
            break;
    }
}

void VirtualMachine::executeMethod(int64_t address) {
    if(address < 0 || address >= manifest.methods) {
        stringstream ss;
        ss << "could not call method @" << address << "; method not found.";
        throw Exception(ss.str());
    }

    Method* method = env->methods+address;
    if(thread_self->callStack.empty())
        thread_self->callStack.add(
                Frame(NULL, 0, 0, 0)); // for main method
    else
        thread_self->callStack.add(
                Frame(thread_self->current, thread_self->pc, registers[sp], registers[fp]));


    thread_self->current = method;
    thread_self->cache = method->bytecode;
    thread_self->cacheSize = method->cacheSize;
    registers[fp] = (registers[sp] - method->paramSize) + 1;
}

int VirtualMachine::returnMethod() {

    Frame frame = thread_self->callStack.last();

    thread_self->current = frame.last;
    thread_self->cache = frame.last->bytecode;
    thread_self->cacheSize=frame.last->cacheSize;

    thread_self->pc = frame.pc;
    registers[sp] = frame.sp;
    registers[fp] = frame.fp;
    thread_self->callStack.pop_back();
    return 0;
}
