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
#include "symbols/Exception.h"
#include "../util/time.h"
#include "Opcode.h"
#include "../grammar/DataType.h"
#include "symbols/Field.h"
#include "Manifest.h"
#include "../Modules/std.io/fileio.h"
#include "../util/File.h"
#include "../Modules/std.kernel/cmath.h"
#include "../Modules/std.kernel/clist.h"
#include "jit/Jit.h"
#include "main.h"

#ifdef WIN32_
#include <conio.h>
#endif
#ifdef POSIX_
#include "termios.h"
#endif

VirtualMachine vm;
int CreateVirtualMachine(string &exe)
{
    int result;
    if((result = Process_Exe(exe)) != 0) {
        if(!exeErrorMessage.empty())
            fprintf(stderr, "%s", exeErrorMessage.c_str());
        return result;
    }

    if((internalStackSize - vm.manifest.threadLocals) <= 1)
        return 2;

    Thread::Startup();
    GarbageCollector::startup();

#ifdef BUILD_JIT
    Jit::startup();
#endif
#ifdef WIN32_
//    vm.gui = new Gui();
//    vm.gui->setupMain();
    vm.gui = NULL;
#endif

    /**
     * Resolve Exception classes
     */
//    vm.Throwable = vm.resolveClass("std#throwable");
//    vm.RuntimeExcept = vm.resolveClass("std#runtime_exception");
//    vm.StackOverflowExcept = vm.resolveClass("std#stack_overflow_exception");
//    vm.ThreadStackExcept = vm.resolveClass("std#thread_stack_exception");
//    vm.IndexOutOfBoundsExcept = vm.resolveClass("std#index_out_of_bounds_exception");
//    vm.NullptrExcept = vm.resolveClass("std#nullptr_exception");
//    vm.ClassCastExcept = vm.resolveClass("std#class_cast_exception");
//    vm.OutOfMemoryExcept = vm.resolveClass("std#out_of_memory_exception");
    cout.precision(16);

    /**
     * Initialize all classes to be used for static access
     */
//    for(unsigned long i = 0; i < vm.manifest.classes; i++) {
//        vm.staticHeap[i].object = GarbageCollector::self->newObject(&vm.classes[i], true);
//        SET_GENERATION(vm.staticHeap[i].object->info, gc_perm);
//    }

    return 0;
}

void invokeDelegate(int64_t address, int32_t args, Thread* thread, int64_t staticAddr) {
    SharpObject* o2 = staticAddr!=0 ? vm.staticHeap[staticAddr].object :  (thread->sp-args)->object.object;
    ClassObject* klass;
    fptr jitFn;
    thread->pc++;

    if(o2!=NULL && TYPE(o2->info) == _stype_struct) {
        klass = &vm.classes[CLASS(o2->info)];
        if (klass != NULL) {
            search:
            for (long i = 0; i < klass->methodCount; i++) {
                if (klass->methods[i]->delegateAddress == address) {
                    if((jitFn = executeMethod(klass->methods[i]->address, thread)) != NULL) {
#ifdef BUILD_JIT
                        jitFn(thread->jctx);
#endif
                    }
                    return;
                }
            }

            if (klass->super != NULL) {
                klass = klass->super;
                goto search;
            }
            throw Exception(vm.RuntimeExcept, "delegate function has no subscribers");
        } else {
            throw Exception(vm.RuntimeExcept, "attempt to call delegate function on non class object");
        }
    } else
        throw Exception(vm.NullptrExcept, "");
}

bool returnMethod(Thread* thread) {
    if(thread->calls == 0) {
#ifdef SHARP_PROF_
        thread->tprof->endtm=Clock::realTimeInNSecs();
                thread->tprof->profile();
#endif
        return true;
    }

    Frame *frameInfo = thread->callStack+(thread->calls);

    thread->current = frameInfo->returnAddress;
    thread->cache = thread->current->bytecode;

   thread->pc = frameInfo->pc;
   thread->sp = frameInfo->sp;
   thread->fp = frameInfo->fp; // TODO: check if exception state is active and verify wether we have to decrement the stack based on the method we are returning from
   thread->calls--;
#ifdef SHARP_PROF_
    thread->tprof->profile();
#endif

    /**
     * We need to return back to the JIT context
     */
    return frameInfo->isjit;
}

fptr executeMethod(int64_t address, Thread* thread, bool inJit) {

    Method *method = vm.methods+address;
    THREAD_STACK_CHECK2(thread, method->stackSize, address);

    thread->callStack[++thread->calls]
            .init(thread->current, thread->pc, thread->sp-method->spOffset, thread->fp, inJit);

    thread->current = method;
    thread->cache = method->bytecode;
    thread->fp = thread->sp - method->fpOffset;
    thread->sp += (method->stackSize - method->paramSize);
    thread->pc = thread->cache;

#ifdef BUILD_JIT
    if(!method->isjit) {
        if(method->branches >= JIT_IR_LIMIT)
        {
            if(!method->compiling && method->jitAttempts < JIT_MAX_ATTEMPTS)
                Jit::sendMessage(method);
        } else method->branches++;
    }
#endif


#ifdef BUILD_JIT
    if(method->isjit) {
        thread->jctx->caller = method;
        return method->jit_func;
    } else
#endif

    if(inJit || thread->calls==0) {
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

#ifdef BUILD_JIT
    Jit::shutdown();
#endif

#ifdef WIN32_
    if(vm.gui != NULL)
    {
        vm.gui->shutdown();
        delete vm.gui;
    }

#endif
    for(uInt i = 0; i < manifest.methods; i++) {
        methods[i].free();
    }
    std::free (methods);
    for(uInt i = 0; i < manifest.strings; i++)
       strings[i].free();
    std::free (strings);

    for(uInt i = 0; i < manifest.classes; i++) {
        classes[i].free();
    }
    std::free (classes);

    for(uInt i = 0; i < manifest.sourceFiles; i++)
        metaData.files.get(i).free();
    metaData.files.free();

    /**
     * The beauty of the memory management system is it does all the hard work for us
     * so all we have to do in the end is just delete the global heap objects the pointers
     * they are refrencing are meaningless at this point
     */
    std::free (this->staticHeap);

    manifest.application.free();
    manifest.version.free();
    metaData.free();
}

extern void printRegs();
extern void printStack();

#ifdef WIN32_
DWORD WINAPI
#endif
#ifdef POSIX_
void*
#endif
VirtualMachine::InterpreterThreadStart(void *arg) {
    Thread *thread = (Thread*)arg;
    thread_self = thread;
    thread->state = THREAD_RUNNING;
    thread->stbase = (int64_t)&arg;
    thread->stfloor = thread->stbase - thread->stackSize;

    fptr jitFn;
    Thread::setPriority(thread, thread->priority);

    try {
        thread_self->setup();
        /*
         * Call main method
         */
        if((jitFn = executeMethod(thread_self->main->address, thread_self)) != NULL) {

#ifdef BUILD_JIT
            jitFn(thread_self->jctx);
#endif
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
    if(vm.state != VM_TERMINATED)
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
        vm.shutdown();
    }

#ifdef WIN32_
    return 0;
#endif
#ifdef POSIX_
    return NULL;
#endif
}

void VirtualMachine::shutdown() {
    if(vm.state != VM_TERMINATED) {
        destroy();
        vm.state = VM_TERMINATED;
    }
}

void VirtualMachine::sysInterrupt(int64_t signal) {
    switch (signal) {
        case 0x9f:
            /* does nothing nop equivalent */
//            throw Exception("");
            return;
        case 0xc7:
            __snprintf((int) registers[EGX], registers[EBX], (int) registers[ECX]);
            return;
        case 0xa0:
            registers[BMR]= Clock::__os_time((int) registers[EBX]);
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
            vm.gui->winGuiIntf(_64EBX);
            break;
#endif
        case 0xa3:
            registers[BMR]= Clock::realTimeInNSecs();
            return;
        case 0xa4: {
            Thread *thread = Thread::getThread((int32_t )registers[ADX]);

            if(thread != NULL) {
                thread->currentThread = (thread_self->sp--)->object;
                thread->args = (thread_self->sp--)->object;
            }
            registers[CMT]=Thread::start((int32_t )registers[ADX], (size_t )registers[EBX]);
            return;
        }
        case 0xa5:
            registers[CMT]=Thread::join((int32_t )registers[ADX]);
            return;
        case 0xa6:
            registers[CMT]=Thread::interrupt((int32_t )registers[ADX]);
            return;
        case 0xa7:
            registers[CMT]=Thread::destroy((int32_t )registers[ADX]);
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
            if(str != NULL && TYPE(str->info) == _stype_var) {
                native_string cmd;
                for(long i = 0; i < str->size; i++)
                    cmd += str->HEAD[i];
                _64CMT= system(cmd.str().c_str());
                cmd.free();
            } else
                throw Exception(vm.NullptrExcept, "");
            return;
        }
        case 0xe9:
            _64CMT= _kbhit();
            return;
        case 0xa8:
            registers[EBX]=Thread::Create((int32_t )registers[ADX], (bool)registers[EBX]);
            return;
        case 0xe4:
            registers[CMT]=Thread::setPriority((int32_t )registers[ADX], (int)registers[EGX]);
            return;
        case 0xe5:
            __os_yield();
            return;
        case 0xe6:
            clist((int)registers[ADX]);
            return;
        case 0xa9:
            vm.shutdown();
            return;
        case 0xaa:
            registers[CMT]=GarbageCollector::self->getMemoryLimit();
            return;
        case 0xab:
            registers[CMT]=GarbageCollector::self->getManagedMemory();
            return;
        case 0xac:
            __os_sleep((int64_t) registers[EBX]);
            return;
        case 0xb0: {
            Object *arry = &thread_self->sp->object;
            SharpObject *o = arry->object;

            if(o != NULL && TYPE(o->info) == _stype_var) {
                native_string path, absolute;
                for(long i = 0; i < o->size; i++) {
                    path += o->HEAD[i];
                }
                absolute = resolve_path(path);
                GarbageCollector::self->createStringArray(arry, absolute);
            } else
                throw Exception(vm.NullptrExcept, "");
            return;
        }
        case 0xc0: {
            size_t len = (thread_self->sp--)->var;
            Object *arry = &thread_self->sp->object;
            SharpObject *o = arry->object;

            if(o != NULL) {
                if(len > o->size || len < 0) {
                    stringstream ss;
                    ss << "invalid call to native Runtime.copy() len: " << len
                       << ", array size: " << o->size;
                    throw Exception(ss.str());
                }

                if(IS_CLASS(o->info)) { // class?

                    if(TYPE(o->info) != _stype_struct || o->node == NULL)
                        throw Exception(vm.NullptrExcept, "");
                    *arry = GarbageCollector::self->newObjectArray(len, &vm.classes[CLASS(o->info)]);

                    for(size_t i = 0; i < len; i++) {
                        arry->object->node[i] = o->node[i];
                    }

                } else if(TYPE(o->info) == _stype_var) { // var[]
                    *arry = GarbageCollector::self->newObject(len);
                    std::memcpy(arry->object->HEAD, o->HEAD, sizeof(double)*len);

                } else if(TYPE(o->info) == _stype_struct && o->node != NULL) { // object? maybe...
                    *arry = GarbageCollector::self->newObject(len);
                    for(size_t i = 0; i < len; i++) {
                        arry->object->node[i] = o->node[i];
                    }
                }

            } else
                throw Exception(vm.NullptrExcept, "");
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
                    ss << "invalid call to native Runtime.copy2() index-len: " << indexLen
                       << ", array size: " << o->size;
                    throw Exception(ss.str());
                }

                if(IS_CLASS(o->info)) { // class?
                    if(TYPE(o->info) != _stype_struct || o->node == NULL)
                        throw Exception(vm.NullptrExcept, "");
                    data = GarbageCollector::self->newObjectArray(len, &vm.classes[CLASS(o->info)]);

                    for(size_t i = 0; i < indexLen; i++) {
                        data.object->node[i] = o->node[i];
                    }

                    *arry = data.object;
                    data.object->refCount = 1;
                } else if(TYPE(o->info) == _stype_var) { // var[]
                    data = GarbageCollector::self->newObject(len);
                    std::memcpy(data.object->HEAD, o->HEAD, sizeof(double)*indexLen);
                    *arry = data.object;
                    data.object->refCount = 1;
                } else if(TYPE(o->info) == _stype_struct && o->node != NULL) { // object? maybe...
                    data = GarbageCollector::self->newObjectArray(len);
                    for(size_t i = 0; i < indexLen; i++) {
                        data.object->node[i] = o->node[i];
                    }

                    *arry = data.object;
                    data.object->refCount = 1;
                }

            } else
                throw Exception(vm.NullptrExcept, "");
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
                    ss << "invalid call to native Runtime.memcpy() startIndex: " << startIndex
                       << " endIndex: " << endIndex << ", array size: " << o->size;
                    throw Exception(ss.str());
                }

                if(IS_CLASS(o->info)) { // class?
                    if(TYPE(o->info) != _stype_struct || o->node == NULL)
                        throw Exception(vm.NullptrExcept, "");
                    data = GarbageCollector::self->newObjectArray(sz+1, &vm.classes[CLASS(o->info)]);

                    for(size_t i = startIndex; i < o->size; i++) {
                        data.object->node[idx++] = o->node[i];
                        if(i==endIndex) break;
                    }

                    *arry = data.object;
                    data.object->refCount = 1;
                } else if(TYPE(o->info) == _stype_var) { // var[]
                    data = GarbageCollector::self->newObject(sz+1);
                    std::memcpy(data.object->HEAD, &o->HEAD[startIndex], sizeof(double)*data.object->size);
                    *arry = data.object;
                    data.object->refCount = 1;
                } else if(TYPE(o->info) == _stype_struct && o->node != NULL) { // object? maybe...
                    data = GarbageCollector::self->newObjectArray(sz+1);

                    for(size_t i = startIndex; i < o->size; i++) {
                        data.object->node[idx++] = o->node[i];
                        if(i==endIndex) break;
                    }

                    *arry = data.object;
                    data.object->refCount = 1;
                }

            } else
                throw Exception(vm.NullptrExcept, "");
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
                    ss << "invalid call to native Runtime.reverse() startIndex: " << startIndex
                       << " endIndex: " << endIndex << ", array size: " << o->size;
                    throw Exception(ss.str());
                }

                if(IS_CLASS(o->info)) { // class?
                    if(o->node == NULL)
                        throw Exception(vm.NullptrExcept, "");
                    data = GarbageCollector::self->newObjectArray(sz+1, &vm.classes[CLASS(o->info)]);

                    for(size_t i = endIndex; i > 0; i--) {
                        data.object->node[idx++] = o->node[i];
                        if(i==startIndex) break;
                    }

                    *arry = data.object;
                    data.object->refCount = 1;
                } else if(TYPE(o->info) == _stype_var) { // var[]
                    data = GarbageCollector::self->newObject(sz+1);

                    for(size_t i = endIndex; i > 0; i--) {
                        data.object->HEAD[idx++] = o->HEAD[i];
                        if(i==startIndex) break;
                    }

                    *arry = data.object;
                    data.object->refCount = 1;
                } else if(TYPE(o->info) == _stype_struct && o->node != NULL) { // object? maybe...
                    data = GarbageCollector::self->newObjectArray(sz+1);

                    for(size_t i = endIndex; i > 0; i--) {
                        data.object->node[idx++] = o->node[i];
                        if(i==startIndex) break;
                    }

                    *arry = data.object;
                    data.object->refCount = 1;
                }

            } else
                throw Exception(vm.NullptrExcept, "");
            return;
        } case 0xc6: {
            size_t len = (thread_self->sp--)->var;
            Object *arry = &thread_self->sp->object;
            SharpObject *o = arry->object;

            if(o != NULL) {
                if(len <= 0) {
                    stringstream ss;
                    ss << "invalid call to native Runtime.realloc() len: " << len
                       << ", array size: " << o->size;
                    throw Exception(ss.str());
                }

                if(IS_CLASS(o->info)) { // class?

                    if(o->node == NULL)
                        throw Exception(vm.NullptrExcept, "");

                    GarbageCollector::self->reallocObject(o, len);
                } else if(TYPE(o->info) == _stype_var) { // var[]
                    GarbageCollector::self->realloc(o, len);
                } else if(TYPE(o->info) == _stype_struct && o->node != NULL) { // object? maybe...
                    GarbageCollector::self->reallocObject(o, len);
                }

            } else
                throw Exception(vm.NullptrExcept, "");
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

            if(o != NULL && TYPE(o->info)==_stype_var) {
                native_string path;
                for(long i = 0; i < o->size; i++) {
                    path += o->HEAD[i];
                }
                if(signal==0xb1)
                    registers[EBX] = check_access(path, (int)registers[EBX]);
                else if(signal==0xb2)
                    registers[EBX] = get_file_attrs(path);
                else if(signal==0xb3)
                    registers[EBX] = last_update(path);
                else if(signal==0xb4)
                    registers[EBX] = file_size(path);
                else if(signal==0xb5)
                    create_file(path);
                else if(signal==0xb6)
                    registers[EBX] = delete_file(path);
                else if(signal==0xb7) {
                    _List<native_string> files;
                    get_file_list(path, files);
                    arry = &(++thread_self->sp)->object;

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
                    registers[EBX] = make_dir(path);
                else if(signal==0xb9)
                    registers[EBX] = delete_dir(path);
                else if(signal==0xbb)
                    registers[EBX] = update_time(path, (time_t)registers[EBX]);
                else if(signal==0xbc)
                    registers[EBX] = __chmod(path, (mode_t)registers[EBX], (bool)registers[EGX], (bool)registers[ECX]);
                else if(signal==0xbf) {
                    File::buffer buf;
                    File::read_alltext(path.str().c_str(), buf);
                    native_string str;
                    arry = &(++thread_self->sp)->object;

                    if(str.injectBuff(buf)) {
                        throw Exception("out of memory");
                    }

                    if(str.len > 0) {
                        GarbageCollector::self->createStringArray(arry, str);
                    } else {
                        GarbageCollector::self->releaseObject(arry);
                    }
                }
            } else
                throw Exception(vm.NullptrExcept, "");
            return;
        }
        case 0xba:
        case 0xbd:  {
            SharpObject *o = (thread_self->sp--)->object.object;
            SharpObject *o2 = (thread_self->sp--)->object.object;

            if (o != NULL && TYPE(o->info) == _stype_var && o2 != NULL && TYPE(o2->info) == _stype_var) {
                native_string path, rename;
                for (long i = 0; i < o->size; i++) {
                    path += o->HEAD[i];
                }
                for (long i = 0; i < o2->size; i++) {
                    rename += o2->HEAD[i];
                }

                if(signal==0xba)
                    registers[EBX] = rename_file(path, rename);
                else if(signal==0xbd) {
                    File::buffer buf;
                    buf.operator<<(rename.str()); // rename will contain our actual unicode data
                    registers[EBX] = File::write(path.str().c_str(), buf);
                }
            }

            return;
        }
        case 0xbe:
            registers[EBX]=disk_space((int32_t )registers[EBX]);
            return;
        case 0xc2:
            registers[EBX] = GarbageCollector::_sizeof((thread_self->sp--)->object.object);
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

    thread->current = returnAddress->returnAddress;
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
            thread_self->throwable.handlingClass = &vm.classes[CLASS(thread_self->exceptionObject.object->info)];
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

    if(method->tryCatchTable.size() > 0) {
//        ExceptionTable *tbl=NULL;
//        for(long int i = 0; i < method->tryCatchTable.len; i++) {
//            ExceptionTable& et = method->tryCatchTable._Data[i];
//
//            if (et.start_pc <= pc && et.end_pc >= pc)
//            {
//                if (tbl == NULL || et.start_pc > tbl->start_pc)
//                    tbl = &et;
//            }
//        }
//
//        if(tbl != NULL)
//        {
//            Thread* self = thread_self;
//            (self->fp+tbl->local)->object = exceptionObject;
//            self->pc = self->cache+tbl->handler_pc;
//            DEC_REF(self->exceptionObject.object);
//
//            // cancel exception we caught it
//            sendSignal(self->signal, tsig_except, 0);
//            startAddress = 0;
//            return true;
//        }
    }

    return false;
}

void VirtualMachine::fillStackTrace(Object *exceptionObject) {
    native_string str;
    fillStackTrace(str);
    thread_self->throwable.stackTrace = str;

    if(exceptionObject->object && IS_CLASS(exceptionObject->object->info)) {

        Object* stackTrace = vm.resolveField("stackTrace", exceptionObject->object);
        Object* message = vm.resolveField("message", exceptionObject->object);

        if(stackTrace != NULL) {
            GarbageCollector::self->createStringArray(stackTrace, str);
        }
        if(message != NULL) {
            if(thread_self->throwable.native) // TODO: no longer required
                GarbageCollector::self->createStringArray(message, thread_self->throwable.message);
            else if(message->object != NULL && TYPE(message->object->info) == _stype_var) {
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
    if(func->sourceFile != -1 && func->sourceFile < vm.manifest.sourceFiles) {
        ss << "\""; ss << vm.metaData.files.get(func->sourceFile).name.str() << "\"";
    }
    else
        ss << "\"Unknown File\"";

    long long x, line=-1, ptr=-1;
    for(x = 0; x < func->lineTable.size(); x++)
    {
        if(func->lineTable.get(x).pc >= (frameInfo.pc - func->bytecode)) {
            if(x > 0)
                ptr = x-1;
            else
                ptr = x;
            break;
        }

        if(!((x+1) < func->lineTable.size())) {
            ptr = x;
        }
    }

    Thread * t = thread_self;
    if(ptr != -1) {
        ss << ", line " << (line = func->lineTable.get(ptr).line_number);
    } else
        ss << ", line ?";

    ss << ", in "; ss << func->fullName.str() << "()";

    if(c_options.debugMode) {
        ss << (frameInfo.isjit ? "[native]" : "") << " [0x" << std::hex
           << func->address << "] $0x" << (frameInfo.pc == 0 ? 0 : (frameInfo.pc-func->bytecode))  << std::dec;

        ss << " fp; " << (frameInfo.fp == 0 ? 0 : frameInfo.fp-thread_self->dataStack) << " sp: " << (frameInfo.sp == 0 ? 0 : frameInfo.sp-thread_self->dataStack);
    }

    if(line != -1 && vm.metaData.files.size() > 0) {
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

        for(long i = 0; i < thread_self->calls; i++) {
            if(iter++ >= EXCEPTION_PRINT_MAX)
                break;
            fillMethodCall(thread_self->callStack[i].returnAddress, thread_self->callStack[i + 1], ss);
        }
    } else {
        for(long long i = (thread_self->calls+1)-EXCEPTION_PRINT_MAX -1; i < thread_self->calls+1; i++) {
            if(iter++ >= EXCEPTION_PRINT_MAX)
                break;
            fillMethodCall(thread_self->callStack[i].returnAddress, thread_self->callStack[i + 1], ss);
        }
    }

    Frame frame(thread_self->current, thread_self->pc, thread_self->sp, thread_self->fp,0);
    fillMethodCall(thread_self->current, frame, ss);

//    Frame frame(thread_self->current, thread_self->pc, thread_self->sp, thread_self->fp,0);
//    fillMethodCall(frame, ss);

    str = ss.str();
}

string VirtualMachine::getPrettyErrorLine(long line, long sourceFile) {
    stringstream ss;
    line -=2;

    if(line >= 0)
        ss << endl << "\t   " << line << ":    "; ss << vm.metaData.getLine(line, sourceFile).str();
    line++;

    if(line >= 0)
        ss << endl << "\t   " << line << ":    "; ss << vm.metaData.getLine(line, sourceFile).str();
    line++;

    ss << endl << "\t>  " << line << ":    "; ss << vm.metaData.getLine(line, sourceFile).str();
    line++;

    if(vm.metaData.hasLine(line, sourceFile))
        ss << endl << "\t   " << line << ":    "; ss << vm.metaData.getLine(line, sourceFile).str();
    line++;

    if(vm.metaData.hasLine(line, sourceFile))
        ss << endl << "\t   " << line << ":    "; ss << vm.metaData.getLine(line, sourceFile).str();
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

    native_string str(string(buf, 256));
    GarbageCollector::self->createStringArray(&(++thread_self->sp)->object, str);
}

Method *VirtualMachine::getMainMethod() {
    return &methods[manifest.entryMethod];
}

ClassObject *VirtualMachine::resolveClass(runtime::String fullName) {
    for(uInt i = 0; i < vm.manifest.classes; i++) {
        if(vm.classes[i].fullName == fullName) {
            return &vm.classes[i];
        }
    }

    return nullptr;
}

Object *VirtualMachine::resolveField(runtime::String name, SharpObject *classObject) {
    if(IS_CLASS(classObject->info)) {
        ClassObject *representedClass = &vm.classes[CLASS(classObject->info)];
        for(Int i = 0; i < representedClass->totalFieldCount; i++) {
            Field &field = representedClass->fields[i];
            if(field.name == name) {
                if(isStaticObject(classObject) == IS_STATIC(field.flags))
                    return &classObject->node[field.address];
                else return nullptr;
            }
        }
    }
    return nullptr;
}

bool VirtualMachine::isStaticObject(SharpObject *object) {
    return object != NULL && GENERATION(object->info) == gc_perm;
}
