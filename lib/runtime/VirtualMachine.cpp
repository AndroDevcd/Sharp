//
// Created by BraxtonN on 2/15/2018.
//

#include <random>
#include <cmath>
#include <string>
#include "VirtualMachine.h"
#include "Exe.h"
#include "Thread.h"
#include "memory/GarbageCollector.h"
#include "register.h"
#include "Environment.h"
#include "../util/time.h"
#include "Opcode.h"
#include "../grammar/FieldType.h"
#include "oo/Field.h"
#include "Manifest.h"
#include "../Modules/std.io/fileio.h"
#include "../util/File.h"
#include "../Modules/std.kernel/cmath.h"
#include "../Modules/std.kernel/clist.h"
#include "Jit.h"
#include "main.h"

#ifdef WIN32_
#include <conio.h>
#endif
#ifdef POSIX_
#include "termios.h"
#endif

VirtualMachine* vm;
Environment* env;
bool masterShutdown = false;

int CreateVirtualMachine(std::string exe)
{
    vm = (VirtualMachine*)__malloc(sizeof(VirtualMachine)*1);
    env = (Environment*)__malloc(sizeof(Environment)*1);

    if(Process_Exe(exe) != 0)
        return 1;

    vm->exitVal = 0;
    if((iStackSz - interp_STACK_start_MIN - manifest.threadLocals) <= 0)
        return 2;

    Thread::Startup();
    GarbageCollector::startup();
    Jit::startup();
#ifdef WIN32_
    env->gui = new Gui();
    env->gui->setupMain();
#endif

    manifest.classes -= AUX_CLASSES;

    /**
     * Aux classes
     */
    Field* fields=(Field*)malloc(sizeof(Field)*2);
    fields[0].init("message", 0, VAR, false, true, &env->classes[manifest.classes]);
    fields[1].init("stackTrace", 0, VAR, false, true, &env->classes[manifest.classes]);

    env->classes[manifest.classes].init();
    env->classes[manifest.classes] = ClassObject(
            "std#Throwable",
            fields,
            2,
            NULL,
            manifest.classes
    );
    env->Throwable = &env->classes[manifest.classes++];

    fields=(Field*)malloc(sizeof(Field)*2);
    fields[0].init("message", 0, VAR, false, true, env->Throwable);
    fields[1].init("stackTrace", 0, VAR, false, true, env->Throwable);

    env->classes[manifest.classes].init();
    env->classes[manifest.classes] = ClassObject(
            "std#RuntimeErr",
            fields,
            2,
            env->Throwable,
            manifest.classes
    );
    env->RuntimeErr = &env->classes[manifest.classes++];

    fields=(Field*)malloc(sizeof(Field)*2);
    fields[0].init("message", 0, VAR, false, true, env->Throwable);
    fields[1].init("stackTrace", 0, VAR, false, true, env->Throwable);

    env->classes[manifest.classes].init();
    env->classes[manifest.classes] = ClassObject(
            "std#StackOverflowErr",
            fields,
            2,
            env->RuntimeErr,
            manifest.classes
    );
    env->StackOverflowErr = &env->classes[manifest.classes++];

    fields=(Field*)malloc(sizeof(Field)*2);
    fields[0].init("message", 0, VAR, false, true, env->Throwable);
    fields[1].init("stackTrace", 0, VAR, false, true, env->Throwable);

    env->classes[manifest.classes].init();
    env->classes[manifest.classes] = ClassObject(
            "std#ThreadStackException",
            fields,
            2,
            env->RuntimeErr,
            manifest.classes
    );
    env->ThreadStackException = &env->classes[manifest.classes++];

    fields=(Field*)malloc(sizeof(Field)*2);
    fields[0].init("message", 0, VAR, false, true, env->Throwable);
    fields[1].init("stackTrace", 0, VAR, false, true, env->Throwable);

    env->classes[manifest.classes].init();
    env->classes[manifest.classes] = ClassObject(
            "std#IndexOutOfBoundsException",
            fields,
            2,
            env->RuntimeErr,
            manifest.classes
    );
    env->IndexOutOfBoundsException = &env->classes[manifest.classes++];

    fields=(Field*)malloc(sizeof(Field)*2);
    fields[0].init("message", 0, VAR, false, true, env->Throwable);
    fields[1].init("stackTrace", 0, VAR, false, true, env->Throwable);

    env->classes[manifest.classes].init();
    env->classes[manifest.classes] = ClassObject(
            "std#NullptrException",
            fields,
            2,
            env->RuntimeErr,
            manifest.classes
    );
    env->NullptrException = &env->classes[manifest.classes++];

    fields=(Field*)malloc(sizeof(Field)*2);
    fields[0].init("message", 0, VAR, false, true, env->Throwable);
    fields[1].init("stackTrace", 0, VAR, false, true, env->Throwable);

    env->classes[manifest.classes].init();
    env->classes[manifest.classes] = ClassObject(
            "std#ClassCastException",
            fields,
            2,
            env->RuntimeErr,
            manifest.classes
    );
    env->ClassCastException = &env->classes[manifest.classes++];

    cout.precision(16);

    /**
     * Initilize all calsses to be used for static access
     * TODO: add flag to check if class has ststic values
     */
    for(unsigned long i = 0; i < manifest.classes-AUX_CLASSES; i++) {
        env->globalHeap[i].object = GarbageCollector::self->newObject(&env->classes[i]);
        env->globalHeap[i].object->gc_info = gc_perm;
    }

    return 0;
}

int64_t executeSwitch(Thread* thread, int64_t constant) {
    register int64_t val;
    if((val = thread->current->switchTable.get(constant).values.indexof(registers[i64ebx])) != -1 ) {
        thread->pc=thread->cache+thread->current->switchTable.get(constant).addresses.get(val);
        return PC(thread);
    } else {
        thread->pc=thread->cache+thread->current->switchTable.get(constant).defaultAddress;
        return PC(thread);
    }
}

void invokeDelegate(int64_t address, int32_t args, Thread* thread, int64_t staticAddr) {
    SharpObject* o2 = staticAddr!=0 ? env->globalHeap[staticAddr].object :  (thread->sp-args)->object.object;
    ClassObject* klass;
    fptr jitFn;

    if(o2!=NULL) {
        klass = o2->k;
        if (klass != NULL) {
            search:
            for (long i = 0; i < klass->methodCount; i++) {
                if (env->methods[klass->methods[i]].delegateAddress == address) {
                    if((thread->calls+1) >= thread->stack_lmt) throw Exception(Environment::StackOverflowErr, "");

                    if((jitFn = executeMethod(env->methods[klass->methods[i]].address, thread)) != NULL) {
                        jitFn(thread->jctx);
                    }
                    return;
                }
            }

            if (klass->base != NULL) {
                klass = klass->base;
                goto search;
            }
            throw Exception(Environment::RuntimeErr, "delegate function has no subscribers");
        } else {
            throw Exception(Environment::RuntimeErr, "attempt to call delegate function on non class object");
        }
    } else
        throw Exception(Environment::NullptrException, "");
}

bool returnMethod(Thread* thread) {
    if(thread->calls <= 0) {
#ifdef SHARP_PROF_
        thread->tprof->endtm=Clock::realTimeInNSecs();
                thread->tprof->profile();
#endif
        return true;
    }

    Frame *frameInfo = thread->callStack+(thread->calls);
    Frame *returnAddress = thread->callStack+(thread->calls-1);

    if(thread->current->finallyBlocks.len > 0) {
        if(vm->executeFinally(thread_self->current)) {
            return false;
        }
    }

    thread->current = returnAddress->current;
    thread->cache = thread->current->bytecode;

   thread->pc = frameInfo->pc;
   thread->sp = frameInfo->sp;
   thread->fp = frameInfo->fp;
    thread->calls--;
#ifdef SHARP_PROF_
    thread->tprof->profile();
#endif
    /**
     * We need to return back to the JIT context
     */
    if(returnAddress->isjit)
        return true;

    return false;
}

fptr executeMethod(int64_t address, Thread* thread, bool inJit) {

    Method *method = env->methods+address;
    StackElement *equlizer=thread->sp-method->stackEqulizer;

    if(thread->calls == -1) {
        thread->callStack[++thread->calls].init(method, 0,0,0, false);
    } else {
        thread->callStack[++thread->calls]
            .init(method, thread->pc, equlizer, thread->fp, false);
    }

    thread->current = method;
    thread->cache = method->bytecode;
    thread->fp = thread->calls==0 ? thread->fp :
                      ((method->returnVal) ? equlizer : (equlizer+1));
    thread->sp += (method->stackSize - method->paramSize);
    THREAD_STACK_CHECK2(thread, address);
    thread->pc = thread->cache;

    if(!method->isjit) {
        if(method->longCalls >= JIT_IR_LIMIT)
        {
            if(!method->compiling && method->jitAttempts < JIT_MAX_ATTEMPTS)
                Jit::sendMessage(method);
        } else method->longCalls++;
    }

    if(method->isjit) {
        thread->callStack[thread->calls].isjit = true;
        thread->jctx->caller = method;
        return method->jit_call;
    } else if(inJit || thread->calls==0) {
        startAddress=0;
        thread->exec();
    }
    return NULL;
}

void VirtualMachine::destroy() {
    if(thread_self != NULL) {
        exitVal = thread_self->exitVal;
    } else
        exitVal = 1;

    Thread::shutdown();
    GarbageCollector::shutdown();
    Jit::shutdown();

#ifdef WIN32_
    if(env->gui != NULL)
    {
        env->gui->shutdown();
        delete env->gui;
    }

#endif
}

extern void printRegs();

#ifdef WIN32_
DWORD WINAPI
#endif
#ifdef POSIX_
void*
#endif
VirtualMachine::InterpreterThreadStart(void *arg) {
    thread_self = (Thread*)arg;
    thread_self->state = THREAD_RUNNING;
    thread_self->stbase = (int64_t)&arg;
    thread_self->stfloor = thread_self->stbase - thread_self->stack;

    fptr jitFn;
//    if(thread_self->id == main_threadid) {
//#ifdef WIN32_
////        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
////        SetPriorityClass(GetCurrentThread(), HIGH_PRIORITY_CLASS);
//#endif
//    }

    try {
        thread_self->setup();
        /*
         * Call main method
         */
        if((jitFn = executeMethod(thread_self->main->address, thread_self)) != NULL) {
            jitFn(thread_self->jctx);
        }
    } catch (Exception &e) {
        //    if(thread_self->exceptionThrown) {
        //        cout << thread_self->throwable.stackTrace.str();
        //    }
        thread_self->throwable = e.getThrowable();
        sendSignal(thread_self->signal, tsig_except, 1);
    }


#ifdef SHARP_PROF_
    if(!masterShutdown)
        thread_self->tprof->dump();
#endif

    if(irCount != 0)
        cout << "instructions executed " << irCount << " overflowed " << overflow << endl;

    /*
     * Check for uncaught exception in thread before exit
     */
    if(!masterShutdown)
        thread_self->exit();
    else {
#ifdef WIN32_
        return 0;
#endif
#ifdef POSIX_
        return NULL;
#endif
    }

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
    if(!masterShutdown) {
        destroy();
        env->shutdown();

        masterShutdown = true;
    }
}

void VirtualMachine::sysInterrupt(int64_t signal) {
    switch (signal) {
        case 0x9f:
            /* does nothing nop equivalent */
//            throw Exception("");
            return;
        case 0xc7:
            __snprintf((int) registers[i64egx], registers[i64ebx], (int) registers[i64ecx]);
            return;
        case 0xa0:
            registers[i64bmr]= Clock::__os_time((int) registers[i64ebx]);
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
        case 0xf0:
            _64CMT=GarbageCollector::self->selfCollect();
            break;
        case 0xf1:
            GarbageCollector::self->sedate();
            break;
        case 0xf2:
            GarbageCollector::self->wake();
            break;
        case 0xf3:
            GarbageCollector::self->kill();
            break;
        case 0xf4:
            _64CMT=GarbageCollector::self->isAwake();
            break;
#ifdef WIN32_
        case 0xf5:
            env->gui->winGuiIntf(_64EBX);
            break;
#endif
        case 0xa3:
            registers[i64bmr]= Clock::realTimeInNSecs();
            return;
        case 0xa4: {
            Thread *thread = Thread::getThread((int32_t )registers[i64adx]);

            if(thread != NULL) {
                thread->currentThread = (thread_self->sp--)->object;
                thread->args = (thread_self->sp--)->object;
            }
            registers[i64cmt]=Thread::start((int32_t )registers[i64adx], (size_t )registers[i64ebx]);
            return;
        }
        case 0xa5:
            registers[i64cmt]=Thread::join((int32_t )registers[i64adx]);
            return;
        case 0xa6:
            registers[i64cmt]=Thread::interrupt((int32_t )registers[i64adx]);
            return;
        case 0xa7:
            registers[i64cmt]=Thread::destroy((int32_t )registers[i64adx]);
            return;
        case 0xe0: // native getCurrentThread()
            THREAD_STACK_CHECK(thread_self);
            (++thread_self->sp)->object = thread_self->currentThread;
            return;
        case 0xe1: // native getCurrentThreadArgs()
            THREAD_STACK_CHECK(thread_self);
            (++thread_self->sp)->object = thread_self->args;
            return;
        case 0xe2: // native setCurrentThread(Thread)
            thread_self->currentThread = (thread_self->sp--)->object;
            return;
        case 0xe3:
            _64CMT=__cmath(_64EBX, _64EGX, (int)_64ECX);
            return;
        case 0xe7:
            _64BMR= __crand((int)_64ADX);
            return;
        case 0xe8: {
            SharpObject* str = (thread_self->sp--)->object.object;
            if(str != NULL && str->HEAD != NULL) {
                native_string cmd;
                for(long i = 0; i < str->size; i++)
                    cmd += str->HEAD[i];
                _64CMT= system(cmd.str().c_str());
                cmd.free();
            } else
                throw Exception(env->NullptrException, "");
            return;
        }
        case 0xe9:
            _64CMT= _kbhit();
            return;
        case 0xa8:
            registers[i64cmt]=Thread::Create((int32_t )registers[i64adx]);
            return;
        case 0xe4:
            registers[i64cmt]=Thread::setPriority((int32_t )registers[i64adx], (int)registers[i64egx]);
            return;
        case 0xe5:
            __os_yield();
            return;
        case 0xe6:
            clist((int)registers[i64adx]);
            return;
        case 0xa9:
            vm->shutdown();
            return;
        case 0xaa:
            registers[i64cmt]=GarbageCollector::self->getMemoryLimit();
            return;
        case 0xab:
            registers[i64cmt]=GarbageCollector::self->getManagedMemory();
            return;
        case 0xac:
            __os_sleep((int64_t) registers[i64ebx]);
            return;
        case 0xb0: {
            Object *arry = &thread_self->sp->object;
            SharpObject *o = arry->object;

            if(o != NULL && o->HEAD!=NULL) {
                native_string path, absolute;
                for(long i = 0; i < o->size; i++) {
                    path += o->HEAD[i];
                }
                absolute = resolve_path(path);
                GarbageCollector::self->createStringArray(arry, absolute);
            } else
                throw Exception(Environment::NullptrException, "");
            return;
        }
        case 0xc0: {
            size_t len = (thread_self->sp--)->var;
            Object *arry = &thread_self->sp->object;
            SharpObject *o = arry->object;
            Object data; data.object = NULL;

            if(o != NULL) {
                if(len > o->size || len < 0) {
                    stringstream ss;
                    ss << "invalid call to native System.copy() len: " << len
                       << ", array size: " << o->size;
                    throw Exception(ss.str());
                }

                if(o->k != NULL) { // class?

                    if(o->node == NULL)
                        throw Exception(Environment::NullptrException, "");
                    data = GarbageCollector::self->newObjectArray(len, o->k);

                    for(size_t i = 0; i < len; i++) {
                        data.object->node[i] = o->node[i];
                    }

                    *arry = data.object;
                    data.object->refCount = 1;
                } else if(o->HEAD != NULL) { // var[]
                    data = GarbageCollector::self->newObject(len);
                    std::memcpy(data.object->HEAD, o->HEAD, sizeof(double)*len);

                    *arry = data.object;
                    data.object->refCount = 1;
                } else if(o->node != NULL) { // object? maybe...
                    data = GarbageCollector::self->newObject(len);
                    for(size_t i = 0; i < len; i++) {
                        data.object->node[i] = o->node[i];
                    }
                    *arry = data.object;
                    data.object->refCount = 1;
                }

            } else
                throw Exception(Environment::NullptrException, "");
            return;
        }
        case 0xc1: {
            size_t len = (thread_self->sp--)->var;
            size_t indexLen = (thread_self->sp--)->var;
            Object *arry = &thread_self->sp->object;
            SharpObject *o = arry->object;
            Object data; data.object = NULL;

            if(o != NULL) {
                if(indexLen > o->size || len < 0 || indexLen < 0 ) {
                    stringstream ss;
                    ss << "invalid call to native System.copy2() index-len: " << indexLen
                       << ", array size: " << o->size;
                    throw Exception(ss.str());
                }

                if(o->k != NULL) { // class?
                    if(o->node == NULL)
                        throw Exception(Environment::NullptrException, "");
                    data = GarbageCollector::self->newObjectArray(len, o->k);

                    for(size_t i = 0; i < indexLen; i++) {
                        data.object->node[i] = o->node[i];
                    }

                    *arry = data.object;
                    data.object->refCount = 1;
                } else if(o->HEAD != NULL) { // var[]
                    data = GarbageCollector::self->newObject(len);
                    std::memcpy(data.object->HEAD, o->HEAD, sizeof(double)*indexLen);
                    *arry = data.object;
                    data.object->refCount = 1;
                } else if(o->node != NULL) { // object? maybe...
                    data = GarbageCollector::self->newObjectArray(len);
                    for(size_t i = 0; i < indexLen; i++) {
                        data.object->node[i] = o->node[i];
                    }

                    *arry = data.object;
                    data.object->refCount = 1;
                }

            } else
                throw Exception(Environment::NullptrException, "");
            return;
        } case 0xc3: {
            size_t endIndex = (thread_self->sp--)->var;
            size_t startIndex = (thread_self->sp--)->var;
            Object *arry = &thread_self->sp->object;
            SharpObject *o = arry->object;
            Object data; data.object = NULL;
            size_t sz = endIndex-startIndex, idx=0;

            if(o != NULL) {
                if(startIndex > o->size || startIndex < 0 || endIndex < 0
                   || endIndex > o->size || endIndex < startIndex) {
                    stringstream ss;
                    ss << "invalid call to native System.memcpy() startIndex: " << startIndex
                       << " endIndex: " << endIndex << ", array size: " << o->size;
                    throw Exception(ss.str());
                }

                if(o->k != NULL) { // class?
                    if(o->node == NULL)
                        throw Exception(Environment::NullptrException, "");
                    data = GarbageCollector::self->newObjectArray(sz+1, o->k);

                    for(size_t i = startIndex; i < o->size; i++) {
                        data.object->node[idx++] = o->node[i];
                        if(i==endIndex) break;
                    }

                    *arry = data.object;
                    data.object->refCount = 1;
                } else if(o->HEAD != NULL) { // var[]
                    data = GarbageCollector::self->newObject(sz+1);
                    std::memcpy(data.object->HEAD, &o->HEAD[startIndex], sizeof(double)*data.object->size);
                    *arry = data.object;
                    data.object->refCount = 1;
                } else if(o->node != NULL) { // object? maybe...
                    data = GarbageCollector::self->newObjectArray(sz+1);

                    for(size_t i = startIndex; i < o->size; i++) {
                        data.object->node[idx++] = o->node[i];
                        if(i==endIndex) break;
                    }

                    *arry = data.object;
                    data.object->refCount = 1;
                }

            } else
                throw Exception(Environment::NullptrException, "");
            return;
        } case 0xc4: {
            size_t endIndex = (thread_self->sp--)->var;
            size_t startIndex = (thread_self->sp--)->var;
            Object *arry = &thread_self->sp->object;
            SharpObject *o = arry->object;
            Object data; data.object = NULL;
            size_t sz = endIndex-startIndex, idx=0;

            if(o != NULL) {
                if(startIndex > o->size || startIndex < 0 || endIndex < 0
                   || endIndex > o->size || endIndex < startIndex) {
                    stringstream ss;
                    ss << "invalid call to native System.reverse() startIndex: " << startIndex
                       << " endIndex: " << endIndex << ", array size: " << o->size;
                    throw Exception(ss.str());
                }

                if(o->k != NULL) { // class?
                    if(o->node == NULL)
                        throw Exception(Environment::NullptrException, "");
                    data = GarbageCollector::self->newObjectArray(sz+1, o->k);

                    for(size_t i = endIndex; i > 0; i--) {
                        data.object->node[idx++] = o->node[i];
                        if(i==startIndex) break;
                    }

                    *arry = data.object;
                    data.object->refCount = 1;
                } else if(o->HEAD != NULL) { // var[]
                    data = GarbageCollector::self->newObject(sz+1);

                    for(size_t i = endIndex; i > 0; i--) {
                        data.object->HEAD[idx++] = o->HEAD[i];
                        if(i==startIndex) break;
                    }

                    *arry = data.object;
                    data.object->refCount = 1;
                } else if(o->node != NULL) { // object? maybe...
                    data = GarbageCollector::self->newObjectArray(sz+1);

                    for(size_t i = endIndex; i > 0; i--) {
                        data.object->node[idx++] = o->node[i];
                        if(i==startIndex) break;
                    }

                    *arry = data.object;
                    data.object->refCount = 1;
                }

            } else
                throw Exception(Environment::NullptrException, "");
            return;
        } case 0xc6: {
            size_t len = (thread_self->sp--)->var;
            Object *arry = &thread_self->sp->object;
            SharpObject *o = arry->object;

            if(o != NULL) {
                if(len <= 0) {
                    stringstream ss;
                    ss << "invalid call to native System.realloc() len: " << len
                       << ", array size: " << o->size;
                    throw Exception(ss.str());
                }

                if(o->k != NULL) { // class?

                    if(o->node == NULL)
                        throw Exception(Environment::NullptrException, "");

                    GarbageCollector::self->reallocObject(o, len);
                } else if(o->HEAD != NULL) { // var[]
                    GarbageCollector::self->realloc(o, len);
                } else if(o->node != NULL) { // object? maybe...
                    GarbageCollector::self->reallocObject(o, len);
                }

            } else
                throw Exception(Environment::NullptrException, "");
            return;
        }
        case 0xb1:
        case 0xb2:
        case 0xb3:
        case 0xb4:
        case 0xb5:
        case 0xb6:
        case 0xb7:
        case 0xb8:
        case 0xb9:
        case 0xbb:
        case 0xbc:
        case 0xbf: {
            Object *arry = &(thread_self->sp--)->object;
            SharpObject *o = arry->object;

            if(o != NULL && o->HEAD!=NULL) {
                native_string path;
                for(long i = 0; i < o->size; i++) {
                    path += o->HEAD[i];
                }
                if(signal==0xb1)
                    registers[i64ebx] = check_access(path, (int)registers[i64ebx]);
                else if(signal==0xb2)
                    registers[i64ebx] = get_file_attrs(path);
                else if(signal==0xb3)
                    registers[i64ebx] = last_update(path);
                else if(signal==0xb4)
                    registers[i64ebx] = file_size(path);
                else if(signal==0xb5)
                    create_file(path);
                else if(signal==0xb6)
                    registers[i64ebx] = delete_file(path);
                else if(signal==0xb7) {
                    _List<native_string> files;
                    get_file_list(path, files);

                    thread_self->sp++;
                    if(files.size()>0) {
                        *arry = GarbageCollector::self->newObjectArray(files.size());
                        o = arry->object;

                        for(long i = 0; i < files.size(); i++) {
                            GarbageCollector::self->createStringArray(&o->node[i], files.get(i));
                            files.get(i).free();
                        }
                    } else {
                        GarbageCollector::self->releaseObject(arry);
                    }

                    files.free();
                }
                else if(signal==0xb8)
                    registers[i64ebx] = make_dir(path);
                else if(signal==0xb9)
                    registers[i64ebx] = delete_dir(path);
                else if(signal==0xbb)
                    registers[i64ebx] = update_time(path, (time_t)registers[i64ebx]);
                else if(signal==0xbc)
                    registers[i64ebx] = __chmod(path, (mode_t)registers[i64ebx], (bool)registers[i64egx], (bool)registers[i64ecx]);
                else if(signal==0xbf) {
                    File::buffer buf;
                    File::read_alltext(path.str().c_str(), buf);
                    native_string str;

                    if(str.injectBuff(buf)) {
                        throw Exception("out of memory");
                    }

                    thread_self->sp++;
                    if(str.len > 0) {
                        GarbageCollector::self->createStringArray(arry, str);
                    } else {
                        GarbageCollector::self->releaseObject(arry);
                    }
                }
            } else
                throw Exception(Environment::NullptrException, "");
            return;
        }
        case 0xba:
        case 0xbd:  {
            SharpObject *o = (thread_self->sp--)->object.object;
            SharpObject *o2 = (thread_self->sp--)->object.object;

            if (o != NULL && o->HEAD != NULL && o2 != NULL && o2->HEAD != NULL) {
                native_string path, rename;
                for (long i = 0; i < o->size; i++) {
                    path += o->HEAD[i];
                }
                for (long i = 0; i < o2->size; i++) {
                    rename += o2->HEAD[i];
                }

                if(signal==0xba)
                    registers[i64ebx] = rename_file(path, rename);
                else if(signal==0xbd) {
                    File::buffer buf;
                    buf.operator<<(rename.str()); // rename will contain our actual unicode data
                    registers[i64ebx] = File::write(path.str().c_str(), buf);
                }
            }

            return;
        }
        case 0xbe:
            registers[i64ebx]=disk_space((int32_t )registers[i64ebx]);
            return;
        case 0xc2:
            registers[i64ebx] = GarbageCollector::_sizeof((thread_self->sp--)->object.object);
            return;
        case 0xd0:
            cout << std::flush;
            break;
        default: {
            // unsupported
            stringstream ss;
            ss << "unsupported signal to int instruction: " << signal;
            throw Exception(ss.str());
        }
    }
}


int VirtualMachine::returnMethod() { // TOTO: change call structure to not hold the last function Info but the current called function on the frame
    Thread *thread = thread_self;
    if(thread->calls <= 0)
        return 1;

    Frame *frameInfo = thread->callStack+(thread->calls);
    Frame *returnAddress = thread->callStack+(thread->calls-1);
    
    if(thread->current->finallyBlocks.size() > 0) {

        if(executeFinally(thread->current))
            return 3;
    }

    thread->current = returnAddress->current;
    thread->cache = thread->current->bytecode;

    thread->pc = frameInfo->pc;
    thread->sp = frameInfo->sp;
    thread->fp = frameInfo->fp;
    thread->calls--;
    return returnAddress->isjit ? 2 : 0;
}

void VirtualMachine::Throw() {
//    if(exceptionObject->object == NULL || exceptionObject->object->k == NULL) {
//        cout << "object is not a class" << endl;
//        return;
//    }

    if (!hasSignal(thread_self->signal, tsig_except))
    {
            thread_self->throwable.throwable = thread_self->exceptionObject.object->k;
            fillStackTrace(&thread_self->exceptionObject);
            sendSignal(thread_self->signal, tsig_except, 1);
    }

    if(TryCatch(thread_self->current, &thread_self->exceptionObject)) {
        return;
    }

    int result = thread_self->calls;
    while(thread_self->calls >= 0) {
        result = returnMethod();
        if(result==1)
            break;
        else if(result == 2 || result == 3)
            return;

        if(TryCatch(thread_self->current, &thread_self->exceptionObject)) {
            return;
        }
    }

    // unhandled exception
    throw Exception(thread_self->throwable);
}

bool VirtualMachine::TryCatch(Method *method, Object *exceptionObject) {
    int64_t pc = PC(thread_self);

    if(exceptionObject->object==NULL) return false;

    if(method->exceptions.size() > 0) {
        ExceptionTable *tbl=NULL;
        for(long int i = 0; i < method->exceptions.len; i++) {
            ExceptionTable& et = method->exceptions._Data[i];

            if (et.start_pc <= pc && et.end_pc >= pc)
            {
                if (tbl == NULL || et.start_pc > tbl->start_pc)
                    tbl = &et;
            }
        }

        if(tbl != NULL)
        {
            Thread* self = thread_self;
            (self->fp+tbl->local)->object = exceptionObject;
            self->pc = self->cache+tbl->handler_pc;
            DEC_REF(self->exceptionObject.object);

            // cancel exception we caught it
            sendSignal(self->signal, tsig_except, 0);
            startAddress = 0;
            return true;
        }
    }

    return false;
}

void VirtualMachine::fillStackTrace(Object *exceptionObject) {
    native_string str;
    fillStackTrace(str);
    thread_self->throwable.stackTrace = str;

    if(exceptionObject->object && exceptionObject->object->k != NULL) {

        Object* stackTrace = env->findField("stackTrace", exceptionObject->object);
        Object* message = env->findField("message", exceptionObject->object);

        if(stackTrace != NULL) {
            GarbageCollector::self->createStringArray(stackTrace, str);
        }
        if(message != NULL) {
            if(thread_self->throwable.native)
                GarbageCollector::self->createStringArray(message, thread_self->throwable.message);
            else if(message->object != NULL && message->object->HEAD != NULL) {
                stringstream ss;
                for(unsigned long i = 0; i < message->object->size; i++) {
                    ss << (char) message->object->HEAD[i];
                }
                thread_self->throwable.message = ss.str();
            }
        }
    }
}

void VirtualMachine::fillMethodCall(Method* func, Frame &frameInfo, stringstream &ss) {
    ss << "\tSource ";
    if(func->sourceFile != -1 && func->sourceFile < manifest.sourceFiles) {
        ss << "\""; ss << env->sourceFiles[func->sourceFile].str() << "\"";
    }
    else
        ss << "\"Unknown File\"";

    long long x, line=-1, ptr=-1;
    for(x = 0; x < func->lineNumbers.size(); x++)
    {
        if(func->lineNumbers.get(x).pc >= (frameInfo.pc-func->bytecode)) {
            if(x > 0)
                ptr = x-1;
            else
                ptr = x;
            break;
        }

        if(!((x+1) < func->lineNumbers.size())) {
            ptr = x;
        }
    }

    Thread * t = thread_self;
    if(ptr != -1) {
        ss << ", line " << (line = func->lineNumbers.get(ptr).line_number);
    } else
        ss << ", line ?";

    ss << ", in "; ss << func->fullName.str() << "()";

    if(c_options.debugMode) {
        ss << (frameInfo.isjit ? "[native]" : "") << " [0x" << std::hex
           << func->address << "] $0x" << (frameInfo.pc == 0 ? 0 : (frameInfo.pc-func->bytecode))  << std::dec;

        ss << " fp; " << (frameInfo.fp == 0 ? 0 : frameInfo.fp-thread_self->dataStack) << " sp: " << (frameInfo.sp == 0 ? 0 : frameInfo.sp-thread_self->dataStack);
    }

    if(line != -1 && metaData.sourceFiles.size() > 0) {
        ss << getPrettyErrorLine(line, func->sourceFile);
    }

    ss << "\n";
}

void VirtualMachine::fillStackTrace(native_string &str) {
// fill message
    if(thread_self->callStack == NULL) return;

    stringstream ss;
    unsigned int iter = 0;
    if((thread_self->calls+1) <= EXCEPTION_PRINT_MAX) {

        for(long i = 0; i < thread_self->calls+1; i++) {
            if(iter++ >= EXCEPTION_PRINT_MAX)
                break;
            fillMethodCall(thread_self->callStack[i].current, thread_self->callStack[i+1], ss);
        }
    } else {
        for(long long i = (thread_self->calls+1)-EXCEPTION_PRINT_MAX -1; i < thread_self->calls+1; i++) {
            if(iter++ >= EXCEPTION_PRINT_MAX)
                break;
            fillMethodCall(thread_self->callStack[i].current, thread_self->callStack[i+1], ss);
        }

        Frame frame(thread_self->current, thread_self->pc, thread_self->sp, thread_self->fp,0);
        fillMethodCall(thread_self->current, frame, ss);
    }

//    Frame frame(thread_self->current, thread_self->pc, thread_self->sp, thread_self->fp,0);
//    fillMethodCall(frame, ss);

    str = ss.str();
}

string VirtualMachine::getPrettyErrorLine(long line, long sourceFile) {
    stringstream ss;
    line -=2;

    if(line >= 0)
        ss << endl << "\t   " << line << ":    "; ss << metaData.getLine(line, sourceFile);
    line++;

    if(line >= 0)
        ss << endl << "\t   " << line << ":    "; ss << metaData.getLine(line, sourceFile);
    line++;

    ss << endl << "\t>  " << line << ":    "; ss << metaData.getLine(line, sourceFile);
    line++;

    if(metaData.hasLine(line, sourceFile))
        ss << endl << "\t   " << line << ":    "; ss << metaData.getLine(line, sourceFile);
    line++;

    if(metaData.hasLine(line, sourceFile))
        ss << endl << "\t   " << line << ":    "; ss << metaData.getLine(line, sourceFile);
    return ss.str();
}

thread_local char buf[256];
void VirtualMachine::__snprintf(int cfmt, double val, int precision) {
    memset(buf, 0, sizeof(char)*256);
    switch(cfmt) {
        case 'd':
        case 'i':
            snprintf(buf, 256, "%d", (int)val);
            break;
        case 'u':
            snprintf(buf, 256, "%u", (unsigned int)val);
            break;
        case 'o':
            snprintf(buf, 256, "%o", (unsigned int)val);
            break;
        case 'x':
            snprintf(buf, 256, "%x", (unsigned int)val);
            break;
        case 'X':
            snprintf(buf, 256, "%X", (unsigned int)val);
            break;
        case 'f':
            if(precision) {
                stringstream ss;
                ss << "%." << precision << 'f';
                snprintf(buf, 256, ss.str().c_str(), val);
            } else
                snprintf(buf, 256, "%f", val);
            break;
        case 'F':
            if(precision) {
                stringstream ss;
                ss << "%." << precision << 'F';
                snprintf(buf, 256, ss.str().c_str(), val);
            } else
                snprintf(buf, 256, "%F", val);
            break;
        case 'e':
            snprintf(buf, 256, "%e", val);
            break;
        case 'E':
            snprintf(buf, 256, "%E", val);
            break;
        case 'g':
            snprintf(buf, 256, "%g", val);
            break;
        case 'G':
            snprintf(buf, 256, "%G", val);
            break;
        case 'a':
            snprintf(buf, 256, "%a", val);
            break;
        case 'A':
            snprintf(buf, 256, "%A", val);
            break;
        case 'c':
            snprintf(buf, 256, "%c", (unsigned int)val);
            break;
        case '%':
            snprintf(buf, 256, "%%");
            break;
        case 'l': {
            native_string str(to_string((long long)val));
            GarbageCollector::self->createStringArray(&(++thread_self->sp)->object, str);
            return;
        }
        case 'L': {
            native_string str(to_string((unsigned long long)val));
            GarbageCollector::self->createStringArray(&(++thread_self->sp)->object, str);
            return;
        }
        default: {
            native_string str(to_string(val));
            GarbageCollector::self->createStringArray(&(++thread_self->sp)->object, str);
            return;
        }


    }

    native_string str(buf, 256);
    GarbageCollector::self->createStringArray(&(++thread_self->sp)->object, str);
}
