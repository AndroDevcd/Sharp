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
#include "../Modules/math/cmath.h"
#include "jit/Jit.h"
#include "main.h"
#include "../Modules/std.io/memory.h"

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
            fprintf(stderr, "%s\n", exeErrorMessage.c_str());
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
    vm.gui = new Gui();
    vm.gui->setupMain();
#endif

    /**
     * Resolve Exception classes
     */
    vm.Throwable = vm.resolveClass("std#throwable");
    vm.RuntimeExcept = vm.resolveClass("std#runtime_exception");
    vm.StackOverflowExcept = vm.resolveClass("std#stack_overflow_exception");
    vm.ThreadStackExcept = vm.resolveClass("std#thread_stack_exception");
    vm.IndexOutOfBoundsExcept = vm.resolveClass("std#out_of_bounds_exception");
    vm.NullptrExcept = vm.resolveClass("std#nullptr_exception");
    vm.ClassCastExcept = vm.resolveClass("std#class_cast_exception");
    vm.OutOfMemoryExcept = vm.resolveClass("std#out_of_memory_exception");
    vm.InvalidOperationExcept = vm.resolveClass("std#invalid_operation_exception");
    vm.StringClass = vm.resolveClass("std#string");
    vm.StackSate = vm.resolveClass("platform.kernel#stack_state");
    vm.ThreadClass = vm.resolveClass("std.io#thread");
    vm.ExceptionClass = vm.resolveClass("std#exception");
    cout.precision(16);

    vm.outOfMemoryExcept.object = NULL;
    vm.outOfMemoryExcept = GarbageCollector::self->newObject(vm.OutOfMemoryExcept);
    native_string message("out of memory");
    GarbageCollector::self->createStringArray(
            vm.resolveField("message", vm.outOfMemoryExcept.object),
            message);

    /**
     * Initialize all classes to be used for static access
     */
    for(unsigned long i = 0; i < vm.manifest.classes; i++) {
        vm.staticHeap[i].object = GarbageCollector::self->newObject(&vm.classes[i], true);
        SET_GENERATION(vm.staticHeap[i].object->info, gc_perm);
    }

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
            for (Int i = klass->methodCount - 1; i >= 0; i--) {
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

   thread->pc = thread->cache+frameInfo->pc;
   thread->sp = frameInfo->sp;
   thread->fp = frameInfo->fp;
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
            .init(thread->current, PC(thread), thread->sp-method->spOffset, thread->fp, inJit);

    thread->current = method;
    thread->cache = method->bytecode;
    thread->fp = thread->sp - method->fpOffset;
    thread->sp += method->frameStackOffset;
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
        thread->setup();
        /*
         * Call main method
         */
        if((jitFn = executeMethod(thread->main->address, thread)) != NULL) {

#ifdef BUILD_JIT
            jitFn(thread->jctx);
#endif
        }
    } catch (Exception &e) {
        sendSignal(thread->signal, tsig_except, 1);
    }


#ifdef SHARP_PROF_
    if(vm.state != VM_TERMINATED)
        thread_self->tprof->dump();
#endif

    if(irCount != 0)
        cout << "instructions executed " << irCount << " overflowed " << overflow << endl;

    /*
     * Check for uncaught exception in thread before exit
     */
    if(vm.state != VM_TERMINATED) {
        if (thread->id == main_threadid) {
            /*
            * Shutdown all running threads
            * and de-allocate all allocated
            * memory. If we do not call join()
            * to wait for all other threads
            * regardless of what they are doing, we
            * stop them.
            */
            vm.shutdown();
        } else {
            thread->exit();

            if(vm.state != VM_SHUTTING_DOWN)
                Thread::destroy(thread);
        }

#ifdef WIN32_
        return 0;
#endif
#ifdef POSIX_
        return NULL;
#endif
    }
    else {
#ifdef WIN32_
        return 0;
#endif
#ifdef POSIX_
        return NULL;
#endif
    }
}

void VirtualMachine::shutdown() {
    if(vm.state != VM_TERMINATED) {
        vm.state = VM_SHUTTING_DOWN;
        destroy();
        vm.state = VM_TERMINATED;
    }
}

void VirtualMachine::getStackTrace() {
    Thread *thread = thread_self;
    SharpObject *frameInfo = (thread->sp)->object.object;

    if(frameInfo) {
        frameInfo->refCount++;
        (thread->sp)->object = GarbageCollector::self->newObject(vm.StringClass);
        fillStackTrace(frameInfo, (thread->sp)->object.object);
        frameInfo->refCount--;
    }
}

void VirtualMachine::getFrameInfo(Object *frameInfo) {
    if(thread_self->callStack == NULL) return;

    if(frameInfo) {
        Thread *thread = thread_self;
        *frameInfo = GarbageCollector::self->newObject(
                vm.StackSate);

        if (frameInfo) {
            Object *methods = frameInfo->object->node;
            Object *pcList = frameInfo->object->node+1;

            Int iter = 0;
            if (methods && pcList) {
                *methods = GarbageCollector::self->newObject(thread->calls + 1);
                *pcList = GarbageCollector::self->newObject(thread->calls + 1);

                if ((thread->calls + 1) < EXCEPTION_PRINT_MAX) {

                    for (Int i = 1; i <= thread_self->calls; i++) {
                        if (i >= EXCEPTION_PRINT_MAX)
                            break;
                        methods->object->HEAD[iter] = thread->callStack[i].returnAddress->address;
                        pcList->object->HEAD[iter] = thread->callStack[i].pc;
                        iter++;
                    }
                } else {
                    for (Int i = (thread->calls + 1) - EXCEPTION_PRINT_MAX; i <= thread->calls; i++) {
                        if (iter >= EXCEPTION_PRINT_MAX)
                            break;
                        methods->object->HEAD[iter] = thread->callStack[i].returnAddress->address;
                        pcList->object->HEAD[iter] = thread->callStack[i].pc;
                        iter++;
                    }
                }

                methods->object->HEAD[iter] = thread->current->address;
                pcList->object->HEAD[iter] = thread->pc - thread->cache;
            }
        }
    }
}

void VirtualMachine::sysInterrupt(int64_t signal) {
    switch (signal) {
        case OP_NOP:
            return;
        case OP_PRINTF:
            __snprintf((int)_64EGX, _64EBX, (int)_64ECX);
            return;
        case OP_OS_TIME:
            _64BMR = Clock::__os_time((int)_64EBX);
            return;
        case OP_GC_EXPLICIT:
            /**
             * Make an explicit call to the garbage collector. This
             * does not garuntee that it will run
             */
            GarbageCollector::self->sendMessage(
                    CollectionPolicy::GC_EXPLICIT);
            return;
        case OP_GC_LOW:
            /**
             * This should only be used in low memory conditions
             * Sending this request will freeze your entire application
             * for an unspecified amount of time
             */
            GarbageCollector::self->sendMessage(
                    CollectionPolicy::GC_LOW);
            return;
        case OP_GC_COLLECT:
            /**
             * This will perform a forced collection on the calling thread
             * As with everything in the GC this does not gaurantee that any objects
             * will be freed ad a reslt.
             *
             * This system call will only work if the garbage collector has been shutdown
             */
            _64CMT=GarbageCollector::self->selfCollect();
            break;
        case OP_GC_SLEEP:
            GarbageCollector::self->sedate();
            break;
        case OP_GC_WAKEUP:
            GarbageCollector::self->wake();
            break;
        case OP_GC_KILL:
            GarbageCollector::self->kill();
            break;
        case OP_GC_STATE:
            _64CMT=GarbageCollector::self->isAwake();
            break;
#ifdef WIN32_
        case OP_GUI:
            vm.gui->winGuiIntf(_64EBX);
            break;
#endif
        case OP_NAN_TIME:
            _64BMR= Clock::realTimeInNSecs();
            return;
        case OP_THREAD_START: {
            Thread *thread = Thread::getThread((int32_t )_64ADX);

            if(thread != NULL) {
                StackElement *se = thread_self->sp;
                thread->currentThread = (thread_self->sp--)->object;
                thread->args = (thread_self->sp--)->object;

                Object *priorityEnum = vm.resolveField("priority", thread->currentThread.object);
                if(priorityEnum != NULL) {
                    thread->priority = (int) vm.numberValue(0, vm.resolveField("ordinal", priorityEnum->object)->object);
                }

                Object *stackSize = vm.resolveField("stack_size", thread->currentThread.object);
                if(stackSize != NULL) {
                    thread->stackSize = (int) vm.numberValue(0, stackSize->object);
                }
            }
            registers[CMT]=Thread::start((int32_t )_64ADX, (size_t )_64EBX);
            return;
        }
        case OP_THREAD_JOIN:
            registers[CMT]=Thread::join((int32_t )_64ADX);
            return;
        case OP_THREAD_INTERRUPT:
            registers[CMT]=Thread::interrupt((int32_t )_64ADX);
            return;
        case OP_THREAD_CREATE:
            registers[EBX]=Thread::Create((int32_t )_64ADX, (bool)_64EBX);
            return;
        case OP_THREAD_PRIORITY:
            registers[CMT]=Thread::setPriority((int32_t )_64ADX, (int)_64EGX);
            return;
        case OP_THREAD_SUSPEND:
            registers[CMT]=Thread::suspendThread((int32_t )_64ADX);
            return;
        case OP_THREAD_UNSUSPEND:
            registers[CMT]=Thread::unSuspendThread((int32_t )_64ADX, (int32_t )_64EBX);
            return;
        case OP_THREAD_WAIT:
            Thread::suspendFor((Int )_64ADX);
            return;
        case OP_THREAD_CURRENT: // native getCurrentThread()
            THREAD_STACK_CHECK(thread_self);
            (++thread_self->sp)->object = thread_self->currentThread;
            return;
        case OP_THREAD_ARGS: // native getCurrentThreadArgs()
            THREAD_STACK_CHECK(thread_self);
            (++thread_self->sp)->object = thread_self->args;
            return;
        case OP_THREAD_SET_CURRENT: // native setCurrentThread(Thread)
            thread_self->currentThread = (thread_self->sp--)->object;
            return;
        case OP_MATH:
            _64CMT=__cmath(_64EBX, _64EGX, (int)_64ECX);
            return;
        case OP_RANDOM:
            _64BMR= __crand((int)_64ADX);
            return;
        case OP_SYSTEM_EXE: {
            SharpObject* str = (thread_self->sp--)->object.object;
            if(str != NULL && TYPE(str->info) == _stype_var && str->HEAD != NULL) {
                native_string cmd(str->HEAD, str->size);
                _64CMT= system(cmd.str().c_str());
                cmd.free();
            } else
                throw Exception(vm.NullptrExcept, "");
            return;
        }
        case OP_KBHIT:
            _64CMT= _kbhit();
            return;
        case OP_YIELD:
            __os_yield();
            return;
        case OP_EXIT:
            thread_self->exitVal = (int)(thread_self->sp--)->var;
            vm.shutdown();
            return;
        case OP_MEMORY_LIMIT:
            registers[CMT]=GarbageCollector::self->getMemoryLimit();
            return;
        case OP_MEMORY:
            registers[CMT]=GarbageCollector::self->getManagedMemory();
            return;
        case OP_ABS_PATH: {
            Object *relPath = &thread_self->sp->object;

            if(relPath->object != NULL && TYPE(relPath->object->info) == _stype_var && relPath->object->HEAD != NULL) {
                native_string path(relPath->object->HEAD, relPath->object->size), absolute;

                absolute = resolve_path(path);
                GarbageCollector::self->createStringArray(relPath, absolute);
                path.free(); absolute.free();
            } else
                throw Exception(vm.NullptrExcept, "");
            return;
        }
        case OP_MEM_COPY: {
            memcopy();
            return;
        }
        case OP_INVERT: {
            invert();
            return;
        } case OP_REALLOC: {
            size_t len = (thread_self->sp--)->var;
            SharpObject *arry = thread_self->sp->object.object;

            if(arry != NULL) {
                if(len <= 0) {
                    stringstream ss;
                    ss << "invalid call to realloc() len: " << len
                       << ", size: " << arry->size;
                    throw Exception(ss.str());
                }

                if(TYPE(arry->info) == _stype_var) { // var[]
                    GarbageCollector::self->realloc(arry, len);
                } else if(TYPE(arry->info) == _stype_struct && arry->node != NULL) { // object? maybe...
                    GarbageCollector::self->reallocObject(arry, len);
                } else
                    throw Exception(vm.NullptrExcept, "");
            } else
                throw Exception(vm.NullptrExcept, "");
            return;
        }
        case OP_FILE_ACCESS:
        case OP_FILE_ATTRS:
        case OP_FILE_UPDATE_TM:
        case OP_FILE_SIZE:
        case OP_FILE_CREATE:
        case OP_FILE_DELETE:
        case OP_GET_FILES:
        case OP_CREATE_DIR:
        case OP_DELETE_DIR:
        case OP_UPDATE_FILE_TM:
        case OP_CHMOD:
        case OP_READ_FILE: {
            Object *arry = &(thread_self->sp--)->object;
            if(arry->object != NULL && arry->object->HEAD != NULL && TYPE(arry->object->info)==_stype_var) {
                native_string path(arry->object->HEAD, arry->object->size);

                if(signal==OP_FILE_ACCESS)
                    registers[EBX] = check_access(path, (int)_64EBX);
                else if(signal==OP_FILE_ATTRS)
                    registers[EBX] = get_file_attrs(path);
                else if(signal==OP_FILE_UPDATE_TM)
                    registers[EBX] = last_update(path, (int)_64EBX);
                else if(signal==OP_FILE_SIZE)
                    registers[EBX] = file_size(path);
                else if(signal==OP_FILE_CREATE)
                    create_file(path);
                else if(signal==OP_FILE_DELETE)
                    registers[EBX] = delete_file(path);
                else if(signal==OP_GET_FILES) {
                    _List<native_string> files;
                    get_file_list(path, files);
                    arry = &(++thread_self->sp)->object;

                    if(files.size()>0) {
                        *arry = GarbageCollector::self->newObjectArray(files.size());

                        for(long i = 0; i < files.size(); i++) {
                            GarbageCollector::self->createStringArray(&arry->object->node[i], files.get(i));
                            files.get(i).free();
                        }
                    }

                    files.free();
                }
                else if(signal==OP_CREATE_DIR)
                    registers[EBX] = make_dir(path);
                else if(signal==OP_DELETE_DIR)
                    registers[EBX] = delete_dir(path);
                else if(signal==OP_UPDATE_FILE_TM)
                    registers[EBX] = update_time(path, (time_t)registers[EBX]);
                else if(signal==OP_CHMOD)
                    registers[EBX] = __chmod(path, (mode_t)registers[EBX], (bool)registers[EGX], (bool)registers[ECX]);
                else if(signal==OP_READ_FILE) {
                    File::buffer buf;
                    File::read_alltext(path.str().c_str(), buf);
                    native_string str;
                    arry = &(++thread_self->sp)->object;

                    if(str.injectBuff(buf)) {
                        throw Exception(vm.OutOfMemoryExcept, "out of memory");
                    }

                    if(str.len > 0) {
                        GarbageCollector::self->createStringArray(arry, str);
                    } else {
                        GarbageCollector::self->releaseObject(arry);
                    }
                }

                path.free();
            } else
                throw Exception(vm.NullptrExcept, "");
            return;
        }
        case OP_RENAME_FILE:
        case OP_WRITE_FILE:  {
            SharpObject *pathObj = (thread_self->sp--)->object.object;
            SharpObject *newNameObj = (thread_self->sp--)->object.object;

            if (pathObj != NULL && TYPE(pathObj->info) == _stype_var && newNameObj != NULL && TYPE(newNameObj->info) == _stype_var
                && pathObj->HEAD != NULL && newNameObj->HEAD != NULL) {
                native_string path(pathObj->HEAD, pathObj->size), rename(newNameObj->HEAD, newNameObj->size);

                if(signal==OP_RENAME_FILE)
                    registers[EBX] = rename_file(path, rename);
                else if(signal==OP_WRITE_FILE) {
                    File::buffer buf;
                    buf.operator<<(rename.str()); // rename will contain our actual unicode data
                    registers[EBX] = File::write(path.str().c_str(), buf);
                }
            } else
                throw Exception(vm.NullptrExcept, "");

            return;
        }
        case OP_DISK_SPACE:
            registers[EBX]=disk_space((int32_t )registers[EBX]);
            return;
        case OP_SIZEOF:
            registers[EBX] = GarbageCollector::_sizeof((thread_self->sp--)->object.object);
            return;
        case OP_FLUSH:
            cout << std::flush;
            break;
        case OP_GET_FRAME_INFO:
            getFrameInfo(&(++thread_self->sp)->object);
            break;
        case OP_GET_STACK_TRACE:
            getStackTrace();
            break;
        case OP_COPY: {
            copy();
            return;
        }
        case OP_CORES: {
            registers[EBX]=std::thread::hardware_concurrency();
            return;
        }
        default: {
            stringstream ss;
            ss << "invalid system interrupt signal: " << signal;
            throw Exception(vm.InvalidOperationExcept, ss.str());
        }
    }
}

bool VirtualMachine::catchException() {
    Thread *thread = thread_self;
    Int pc = PC(thread_self);
    TryCatchData *tbl=NULL;
    ClassObject *handlingClass = &vm.classes[CLASS(thread->exceptionObject.object->info)];
    for(Int i = 0; i < thread->current->tryCatchTable.len; i++) {
        tbl = &thread->current->tryCatchTable._Data[i];

        if (pc >= tbl->block_start_pc && pc <= tbl->block_end_pc)
        {
            for(Int j = 0; j < tbl->catchTable.len; j++) {
                if(handlingClass->guid == tbl->catchTable._Data[j].caughtException->guid
                    || tbl->catchTable._Data[j].caughtException == vm.ExceptionClass
                    || tbl->catchTable._Data[j].caughtException == vm.Throwable) {
                    if(tbl->catchTable._Data[j].localFieldAddress >= 0)
                        (thread->fp+tbl->catchTable._Data[j].localFieldAddress)->object = thread->exceptionObject;
                    thread->pc = thread->cache+tbl->catchTable._Data[j].handler_pc;

                    DEC_REF(thread->exceptionObject.object);
                    thread->exceptionObject.object = NULL;
                    sendSignal(thread->signal, tsig_except, 0);
                    return true;
                }
            }
        }
    }

    // Since we couldn't catch it we must jump to the first finally block
    for(Int i = 0; i < thread->current->tryCatchTable.len; i++) {
        tbl = &thread->current->tryCatchTable._Data[i];

        if (tbl->try_start_pc <= pc && tbl->try_end_pc >= pc)
        {
            if(tbl->finallyData != NULL) {
                thread->pc = thread->cache+tbl->finallyData->start_pc;
                (thread->fp+tbl->finallyData->exception_object_field_address)->object = thread->exceptionObject;
                sendSignal(thread->signal, tsig_except, 0);
                return true;
            }
        }
    }

    return false;
}

void VirtualMachine::fillStackTrace(SharpObject *frameInfo, SharpObject *stackTrace) {
    if(frameInfo && stackTrace) {
        Object *methods = vm.resolveField("methods", frameInfo);
        Object *pcList = vm.resolveField("pc", frameInfo);
        Object *data = vm.resolveField("data", stackTrace);

        if(data && methods && pcList) {
            native_string str;
            fillStackTrace(methods, pcList, data);
        }
    }
}

void VirtualMachine::fillMethodCall(Method* func, Int pc, stringstream &ss) {
    ss << "\tSource ";
    if(func->sourceFile != -1 && func->sourceFile < vm.manifest.sourceFiles) {
        ss << "\""; ss << vm.metaData.files.get(func->sourceFile).name.str() << "\"";
    }
    else
        ss << "\"Unknown File\"";

    Int x, line=-1, ptr=-1;
    for(x = 0; x < func->lineTable.size(); x++)
    {
        if(func->lineTable.get(x).pc >= pc) {
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
        ss << " ["
           << func->address << "] $" << pc;
    }

    if(line != -1 && vm.metaData.files.size() > 0) {
        ss << getPrettyErrorLine(line, func->sourceFile);
    }

    ss << "\n";
}

void VirtualMachine::fillStackTrace(Object *methods, Object *pcList, Object *data) {
    if(methods && pcList && methods->object && pcList->object && TYPE(methods->object->info) == _stype_var
        && TYPE(pcList->object->info) == _stype_var && data) {

        native_string str;
        stringstream ss;
        for(Int i = 0; i < methods->object->size; i++) {
            fillMethodCall(&vm.methods[(Int)methods->object->HEAD[i] % vm.manifest.methods], (Int)pcList->object->HEAD[i], ss);
        }

        str = ss.str(); ss.str("");
        GarbageCollector::self->createStringArray(data, str);
        str.free();
    }
}

void VirtualMachine::fillStackTrace(native_string &str) {
// fill message
    if(thread_self->callStack == NULL) return;

    stringstream ss;
    uInt iter = 0;
    if((thread_self->calls+1) < EXCEPTION_PRINT_MAX) {

        for(Int i = 1; i <= thread_self->calls; i++) {
            if(iter++ >= EXCEPTION_PRINT_MAX)
                break;
            fillMethodCall(thread_self->callStack[i].returnAddress, thread_self->callStack[i].pc, ss);
        }
    } else {
        for(Int i = (thread_self->calls+1) - EXCEPTION_PRINT_MAX; i < thread_self->calls+1; i++) {
            if(iter++ >= EXCEPTION_PRINT_MAX)
                break;
            fillMethodCall(thread_self->callStack[i].returnAddress, thread_self->callStack[i].pc, ss);
        }
    }

    fillMethodCall(thread_self->current, PC(thread_self), ss);
    str = ss.str(); ss.str("");
}

string VirtualMachine::getPrettyErrorLine(long line, long sourceFile) {
    stringstream ss;
    line -=2;

    if(line >= 0) {
        ss << endl << "\t   " << line << ":    ";
        ss << vm.metaData.getLine(line, sourceFile).str();
    }
    line++;

    if(line >= 0) {
        ss << endl << "\t   " << line << ":    ";
        ss << vm.metaData.getLine(line, sourceFile).str();
    }
    line++;

    ss << endl << "\t>  " << line << ":    "; ss << vm.metaData.getLine(line, sourceFile).str();
    line++;

    if(vm.metaData.hasLine(line, sourceFile)) {
        ss << endl << "\t   " << line << ":    ";
        ss << vm.metaData.getLine(line, sourceFile).str();
    }
    line++;

    if(vm.metaData.hasLine(line, sourceFile)) {
        ss << endl << "\t   " << line << ":    ";
        ss << vm.metaData.getLine(line, sourceFile).str();
    }
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
            native_string str(to_string((Int)val));
            GarbageCollector::self->createStringArray(&(++thread_self->sp)->object, str); str.free();
            return;
        }
        case 'L': {
            native_string str(to_string((uInt)val));
            GarbageCollector::self->createStringArray(&(++thread_self->sp)->object, str);  str.free();
            return;
        }
        default: {
            native_string str(to_string(val));
            GarbageCollector::self->createStringArray(&(++thread_self->sp)->object, str); str.free();
            return;
        }


    }

    native_string str(buf);
    GarbageCollector::self->createStringArray(&(++thread_self->sp)->object, str); str.free();
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
                if(isStaticObject(classObject) == IS_STATIC(field.flags)) {
                    name.free();
                    return &classObject->node[field.address];
                }
                else {
                    name.free();
                    return nullptr;
                }
            }
        }
    }

    name.free();
    return nullptr;
}

bool VirtualMachine::isStaticObject(SharpObject *object) {
    return object != NULL && GENERATION(object->info) == gc_perm;
}

double VirtualMachine::numberValue(Int index, SharpObject *object) {
    if(object != NULL) {
        if (TYPE(object->info) == _stype_var) {
            if(index < object->size)
                return object->HEAD[index];
            else {
                stringstream ss;
                ss << "access to field at index: " << index << " with size of: " << object->size;
                throw Exception(vm.IndexOutOfBoundsExcept, ss.str());
            }
        }
    }

    return 0;
}

string VirtualMachine::stringValue(SharpObject *object) {
    stringstream ss;
    if(object != NULL) {
        if (TYPE(object->info) == _stype_var) {
            for(Int i = 0; i < object->size; i++) {
                ss << (char)object->HEAD[i];
            }
        }
    }
    return ss.str();
}

void VirtualMachine::setFieldVar(runtime::String name, SharpObject *classObject, Int index, double value) {
    Object *field = vm.resolveField(name, classObject);
    if(field != NULL && field->object && TYPE(field->object->info) == _stype_var) {
        if(index < field->object->size)
            field->object->HEAD[index] = value;
        else {
            stringstream ss;
            ss << "access to field at index: " << index << " with size of: " << field->object->size;
            throw Exception(vm.IndexOutOfBoundsExcept, ss.str());
        }
    }
}

void VirtualMachine::setFieldClass(runtime::String name, SharpObject *classObject, ClassObject *klass) {
    Object *field = vm.resolveField(name, classObject);
    if(field != NULL) {
        *field = GarbageCollector::self->newObject(klass);
    }
}
