//
// Created by BraxtonN on 2/15/2018.
//

#include <random>
#include <cmath>
#include <string>
#include <utility>
#include "VirtualMachine.h"
#include "Exe.h"
#include "Thread.h"
#include "memory/GarbageCollector.h"
#include "register.h"
#include "symbols/Exception.h"
#include "../util/time.h"
#include "Opcode.h"
#include "symbols/Field.h"
#include "Manifest.h"
#include "../Modules/std.io/fileio.h"
#include "../util/File.h"
#include "../Modules/math/cmath.h"
#include "main.h"
#include "../Modules/std.io/memory.h"
#include "snb/snb.h"
#include "scheduler/scheduler.h"
#include "../Modules/std.io/serialization.h"
#include "scheduler/thread_controller.h"
#include "scheduler/thread_exception_controller.h"
#include "scheduler/task_controller.h"
#include "scheduler/idle_scheduler.h"

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

    create_main_thread();
    GarbageCollector::startup();
    create_idle_scheduler();

#ifdef BUILD_JIT
    Jit::startup();
#endif
#ifdef WIN32_
    vm.gui = new Gui();
    vm.gui->setupMain();
#endif

    /**
     * Resolve Frequently Used classes
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
    vm.UnsatisfiedLinkExcept = vm.resolveClass("std#unsatisfied_link_error");
    vm.IllStateExcept = vm.resolveClass("std#illegal_state_exception");
    vm.IncompatibleClassExcept = vm.resolveClass("std#incompatible_class_exception");
    vm.ObjectImportError = vm.resolveClass("std#object_import_error");
    vm.StringClass = vm.resolveClass("std#string");
    vm.StackSate = vm.resolveClass("platform.kernel#stack_state");
    vm.ThreadClass = vm.resolveClass("std.io#thread");
    vm.ExceptionClass = vm.resolveClass("std#exception");
    vm.ErrorClass = vm.resolveClass("std#error");
    vm.FiberClass = vm.resolveClass("std.io.fiber#fiber");
    cout.precision(16);

    vm.outOfMemoryExcept.object = NULL;
    vm.outOfMemoryExcept = gc.newObject(vm.OutOfMemoryExcept);
    std::string message = ("out of memory");
    gc.createStringArray(
            vm.resolveField("message", vm.outOfMemoryExcept.object),
            message);

    /**
     * Initialize all classes to be used for static access
     */
    for(unsigned long i = 0; i < vm.manifest.classes; i++) {
        vm.staticHeap[i] = gc.newObject(&vm.classes[i], true);
        SET_GENERATION(vm.staticHeap[i].object->info, gc_perm);
    }

    return 0;
}

void invokeDelegate(int64_t address, int32_t args, Thread* thread, bool isStatic, bool inJit) {
    ClassObject* klass = NULL;
    fptr jitFn;
    thread->this_fiber->pc++;

    if(isStatic) {
        if(vm.methods[address].nativeFunc) {
            if(vm.methods[address].bridge != NULL) {
                setupMethodStack(address, thread_self, true);
                vm.methods[address].bridge(vm.methods[address].linkAddr);
                returnMethod(thread_self);
            } else {
                vm.locateBridgeAndCross(&vm.methods[address]);
            }

            thread->this_fiber->pc++;
            return;
        } else {
            throw Exception(vm.RuntimeExcept, "attempting to call non-native static delegate function");
        }
    } else {
        SharpObject* obj = (thread->this_fiber->sp-args)->object.object;
        if(obj != NULL && TYPE(obj->info) == _stype_struct)
            klass = &vm.classes[CLASS(obj->info)];
    }

    if(klass) {
        if (klass != NULL) {
            search:
            for (Int i = klass->methodCount - 1; i >= 0; i--) {
                if (klass->methods[i]->delegateAddress == address) {
                    executeMethod(klass->methods[i]->address, thread, inJit);
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
    if(thread->this_fiber->calls == 0) {
        thread->this_fiber->finished = true;
#ifdef SHARP_PROF_
        thread->tprof->endtm=Clock::realTimeInNSecs();
        thread->tprof->profile();
#endif
        return true;
    }

    Frame *frameInfo = thread->this_fiber->callStack+(thread->this_fiber->calls);

    thread->this_fiber->current =  &vm.methods[frameInfo->returnAddress];

   thread->this_fiber->cache = thread->this_fiber->current->bytecode;
   thread->this_fiber->pc = frameInfo->pc;
   thread->this_fiber->sp = thread->this_fiber->dataStack+frameInfo->sp;
   thread->this_fiber->fp = thread->this_fiber->dataStack+frameInfo->fp;
   thread->this_fiber->calls--;
#ifdef SHARP_PROF_
    thread->tprof->profile();
#endif

    return false;
}

CXX11_INLINE
void setupMethodStack(int64_t address, Thread* thread, bool inJit) {
    Method *method = vm.methods+address;
    grow_stack3(thread, method->stackSize)
    THREAD_STACK_CHECK2(thread, method->stackSize, address);
    if(thread->this_fiber->calls+1 >= thread->this_fiber->frameSize)
        thread->this_fiber->growFrame();

    thread->this_fiber->callStack[++thread->this_fiber->calls]
            .init(thread->this_fiber->current, thread->this_fiber->pc, (thread->this_fiber->sp-method->spOffset)-thread->this_fiber->dataStack,
                    thread->this_fiber->fp-thread->this_fiber->dataStack);

    thread->this_fiber->current = method;
    thread->this_fiber->fp = thread->this_fiber->sp - method->fpOffset;
    thread->this_fiber->sp += method->frameStackOffset;
    thread->this_fiber->cache = method->bytecode;
    thread->this_fiber->pc = method->bytecode;
}

void executeMethod(int64_t address, Thread* thread, bool inNativeEnv) {
    if(address >= vm.manifest.methods || address < 0){
        stringstream ss;
        ss << "invalid call to method with address of " << address;
        throw Exception(ss.str());
    }

    setupMethodStack(address, thread, inNativeEnv);
#ifdef SHARP_PROF_
    thread->tprof->hit(method);
#endif
    if(inNativeEnv || thread->this_fiber->calls==0) {
        thread->exec();
    }
}

void VirtualMachine::destroy() {
    if(vm.state == VM_CREATED)
        return;

    if(thread_self != NULL) {
        exitVal = thread_self->this_fiber->exitVal;
    } else
        exitVal = 1;

    kill_all_threads();
    GarbageCollector::shutdown();

    for(Int i = 0; i < vm.libs.size(); i++) {
        free_lib(vm.libs.get(i).handle);
        vm.libs.get(i).name.clear();
    }

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
    thread->stbase = (int64_t)&arg;
    thread->stfloor = thread->stbase - thread->stackSize;
    bool initialSetup = true;

    fptr jitFn;
    set_thread_priority(thread, thread->priority);

    retry:
    try {
        if(initialSetup) {
            initialSetup = false;
            setup_thread(thread);
            thread->state = THREAD_RUNNING;
            set_attached_thread(thread->this_fiber, thread);
            set_task_state(thread, thread->this_fiber, FIB_RUNNING, NO_DELAY);
            registers = thread->this_fiber->registers;

            /*
             * Call main method
             */
            executeMethod(thread->this_fiber->main->address, thread);
        } else {
            registers = thread->this_fiber->registers;
            if(thread->this_fiber->state == FIB_RUNNING && !thread->this_fiber->finished) {
                if(thread->this_fiber->calls == -1) {

                    /*
                     * Call main method
                     */
                    executeMethod(thread->this_fiber->main->address, thread);
                } else {
                    thread->exec();
                }
            }
        }

        if(thread->this_fiber && thread->this_fiber->boundThread != thread
           && hasSignal(thread->signal, tsig_except)) {
            print_exception(thread);
        }

        if(thread->state != THREAD_KILLED && !hasSignal(thread->signal, tsig_except)) {
            wait_for_posted_task(thread);

            if(thread->state == THREAD_KILLED || hasSignal(thread->signal, tsig_kill))
                goto end;

            Int fibersLeft = bound_task_count(thread);
            if (fibersLeft == 0 || (fibersLeft == 1 && thread->this_fiber->finished && thread->this_fiber->boundThread == thread))
                goto end;
            else
                goto retry;
        }

    } catch (Exception &e) {
        if(thread->state == THREAD_STARTED && thread->currentThread.object) {
            vm.setFieldVar("exited", thread->currentThread.object, 0, 0); // update respective values
            vm.setFieldVar("id", thread->currentThread.object, 0, -1);
        }

        if(e.getThrowable().handlingClass == vm.OutOfMemoryExcept && thread->state == THREAD_CREATED) {
            GUARD(thread->mutex)
            sendSignal(thread->signal, tsig_kill, 1);
        }

        sendSignal(thread->signal, tsig_except, 1);
        if(thread->this_fiber && thread->this_fiber->boundThread != thread) {
            print_exception(thread);
            wait_for_posted_task(thread);

            if(thread->state == THREAD_KILLED || hasSignal(thread->signal, tsig_kill))
                goto end;

            Int fibersLeft = bound_task_count(thread);
            if (fibersLeft == 0 || (fibersLeft == 1 && thread->this_fiber->finished && get_bound_thread(thread->this_fiber) == thread))
                goto end;
            else
                goto retry;
        }
    }

    end:
#ifdef SHARP_PROF_
    if(vm.state != VM_TERMINATED)
        thread_self->tprof->dump();
#endif

    kill_bound_fibers(thread);
    if(irCount != 0)
        cout << "instructions executed " << irCount << " overflowed " << overflow << endl;

    /*
     * Check for uncaught exception in thread before exit
     */
    if(vm.state != VM_TERMINATED) {
        shutdown_thread(thread);

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
    bool destroySystem = false;
    {
        GUARD(vm.mutex) // prevent multiple threads from shutting down the vm

        if (vm.state == VM_RUNNING) {
            vm.state = VM_SHUTTING_DOWN;
            destroySystem = true;
        }
    }

    if(destroySystem) {
        destroy();
        vm.state = VM_TERMINATED;
    }
}

void VirtualMachine::getStackTrace() {
    Thread *thread = thread_self;
    SharpObject *frameInfo = (thread->this_fiber->sp)->object.object;

    if(frameInfo) {
        INC_REF(frameInfo);
        (thread->this_fiber->sp)->object = gc.newObject(vm.StringClass);
        fillStackTrace(frameInfo, (thread->this_fiber->sp)->object.object);
        DEC_REF(frameInfo);
    }
}

void VirtualMachine::getFrameInfo(Object *frameInfo) {
    if(thread_self->this_fiber->callStack == NULL) return;

    if(frameInfo) {
        Thread *thread = thread_self;
        *frameInfo = gc.newObject(
                vm.StackSate);

        if (frameInfo) {
            Object *methods = vm.resolveField("methods", frameInfo->object);
            Object *pcList = vm.resolveField("pc", frameInfo->object);

            Int iter = 0;
            if (methods && pcList) {
                Int size = (thread->this_fiber->calls + 1) < EXCEPTION_PRINT_MAX ? (thread->this_fiber->calls + 1) : EXCEPTION_PRINT_MAX + 1;
                Field *field = vm.StackSate->getfield("methods");
                *methods = gc.newObject(size, field->type < type_function_ptr ? field->type : NTYPE_VAR);

                field = vm.StackSate->getfield("pc");
                *pcList = gc.newObject(size, field->type < type_function_ptr ? field->type : NTYPE_VAR);

                if ((thread->this_fiber->calls + 1) < EXCEPTION_PRINT_MAX) {

                    for (Int i = 1; i <= thread_self->this_fiber->calls; i++) {
                        if (i >= EXCEPTION_PRINT_MAX)
                            break;
                        methods->object->HEAD[iter] =  vm.methods[thread->this_fiber->callStack[i].returnAddress].address;
                        pcList->object->HEAD[iter] = thread->this_fiber->callStack[i].pc-thread->this_fiber->cache;
                        iter++;
                    }
                } else {
                    for (Int i = (thread->this_fiber->calls + 1) - EXCEPTION_PRINT_MAX; i <= thread->this_fiber->calls; i++) {
                        if (iter >= EXCEPTION_PRINT_MAX)
                            break;
                        else if(i==0) continue;
                        methods->object->HEAD[iter] =  vm.methods[thread->this_fiber->callStack[i].returnAddress].address;
                        pcList->object->HEAD[iter] = thread->this_fiber->callStack[i].pc-thread->this_fiber->cache;
                        iter++;
                    }
                }

                methods->object->HEAD[iter] = thread->this_fiber->current->address;
                pcList->object->HEAD[iter] = thread->this_fiber->pc-thread->this_fiber->cache;
            }
        }
    }
}

extern void populateString(string& s, long double* arry, uInt len) {
    for(Int i = 0; i < len; i++)
        s += (unsigned char)arry[i];
}

void VirtualMachine::sysInterrupt(int64_t signal) {
    switch (signal) {
        case OP_NOP:
            return;
        case OP_PRINTF:
            __snprintf((int)_64EGX, _64EBX, (int)_64ECX);
            return;
        case OP_STRTOD: {
            SharpObject *str = (thread_self->this_fiber->sp--)->object.object;
            if (str != NULL && TYPE(str->info) == _stype_var && str->HEAD != NULL) {
                string num_str;
                populateString(num_str, str->HEAD, str->size);
                _64EBX = strtod(num_str.c_str(), NULL);
            } else
                throw Exception(vm.NullptrExcept, "");
            return;
        }
        case OP_OS_TIME:
            _64BMR = Clock::__os_time((int)_64EBX);
            return;
        case OP_GC_EXPLICIT:
            /**
             * Make an explicit call to the garbage collector. This
             * does not garuntee that it will run
             */
            gc.sendMessage(
                    CollectionPolicy::GC_EXPLICIT);
            return;
        case OP_GC_LOW:
            /**
             * This should only be used in low memory conditions
             * Sending this request will freeze your entire application
             * for an unspecified amount of time
             */
            gc.sendMessage(
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
            _64CMT=gc.selfCollect();
            break;
        case OP_GC_SLEEP:
            gc.sedate();
            break;
        case OP_GC_WAKEUP:
            gc.wake();
            break;
        case OP_GC_KILL:
            gc.kill();
            break;
        case OP_GC_STATE:
            _64CMT=gc.isAwake();
            break;
#ifdef WIN32_
        case OP_GUI:
            vm.gui->winGuiIntf(_64EBX);
            break;
#endif
        case OP_NAN_TIME:
            _64BMR= Clock::realTimeInNSecs();
            return;
        case OP_DELAY:
            delay_task(_64EBX, true);
            return;
        case OP_FIBER_START: {
            SharpObject *name = (thread_self->this_fiber->sp--)->object.object;
            Int mainFunc = (thread_self->this_fiber->sp--)->var;
            Int threadid = (thread_self->this_fiber->sp--)->var;
            _64EBX = -1; // default Id

            if(name != NULL && TYPE(name->info) == _stype_var && name->HEAD != NULL) {
                string fiberName;
                populateString(fiberName, name->HEAD, name->size);
                fiber *fib;

                try {
                    fib = create_task(fiberName, &vm.methods[mainFunc % vm.manifest.methods]);
                } catch(Exception &e) {
                    fiberName.clear();
                    throw;
                }

                Thread *t = get_thread(threadid);
                fib->fiberObject = (thread_self->this_fiber->sp--)->object;
                (++fib->sp)->object = (thread_self->this_fiber->sp--)->object; // apply args to fiber's stack
                bind_task(fib, t);
                set_task_state(NULL, fib, FIB_SUSPENDED, NO_DELAY);

                _64EBX = fib->id;
            } else {
                throw Exception(vm.NullptrExcept, "");
            }
            return;
        }
        case OP_FIBER_SUSPEND: {
            Int fiberId = (thread_self->this_fiber->sp--)->var;
            _64EBX = suspend_task(fiberId);
            return;
        }
        case OP_FIBER_UNSUSPEND: {
            Int fiberId = (thread_self->this_fiber->sp--)->var;
            _64EBX = unsuspend_task(fiberId);
            return;
        }
        case OP_FIBER_KILL: {
            Int fiberId = (thread_self->this_fiber->sp--)->var;
            _64EBX = kill_task(fiberId);
            return;
        }
        case OP_FIBER_BIND: {
            Int fiberId = (thread_self->this_fiber->sp--)->var;
            Int threadId = (thread_self->this_fiber->sp--)->var;
            fiber *fib = locate_task(fiberId);
            _64EBX = 2;

            if(fib) {
                _64EBX = bind_task(fib, get_thread(threadId));
            }
            return;
        }
        case OP_FIBER_BOUND_COUNT: {
            Int threadId = (thread_self->this_fiber->sp--)->var;
            Thread *thread = get_thread(threadId);
            _64EBX = thread ? thread->boundFibers : 0;
            return;
        }
        case OP_FIBER_STATE: {
            Int fiberId = (thread_self->this_fiber->sp--)->var;
            fiber *fib = locate_task(fiberId);
            _64EBX = -1;

            if(fib) {
                _64EBX = get_state(fib);
            }
            return;
        }
        case OP_FIBER_CURRENT: {
            grow_stack2(thread_self)
            THREAD_STACK_CHECK(thread_self);
            (++thread_self->this_fiber->sp)->object
               = thread_self->this_fiber->fiberObject;
            return;
        }
        case OP_THREAD_START: {
            Thread *thread = get_thread((int32_t )_64ADX);

            if(thread != NULL) {
                thread->currentThread = (thread_self->this_fiber->sp--)->object;
                thread->args = (thread_self->this_fiber->sp--)->object;

                Object *priorityEnum = vm.resolveField("priority", thread->currentThread.object);
                if(priorityEnum != NULL) {
                    thread->priority = (int) vm.numberValue(0, vm.resolveField("ordinal", priorityEnum->object)->object);
                }

                Object *stackSize = vm.resolveField("stack_size", thread->currentThread.object);
                if(stackSize != NULL) {
                    thread->stackSize = (int) vm.numberValue(0, stackSize->object);
                }

                Object *name = vm.resolveField("name", thread->currentThread.object);
                if(name != NULL) {
                    Object *data = vm.resolveField("data", name->object);

                    if(data) {
                        string threadName = vm.stringValue(data->object);
                        if(threadName != "") {
                            thread->name = threadName;
                        }
                    }
                }
            }
            registers[CMT]=start_thread((int32_t )_64ADX, (size_t )_64EBX);
            return;
        }
        case OP_STRTOL: {
            Int baseNum = (thread_self->this_fiber->sp--)->var;
            SharpObject *str = (thread_self->this_fiber->sp--)->object.object;
            if(str != NULL && TYPE(str->info) == _stype_var && str->HEAD != NULL) {
                string numStr;
                populateString(numStr, str->HEAD, str->size);
                _64EBX = strtoll(numStr.c_str(), NULL, baseNum);
            } else {
                throw Exception(vm.NullptrExcept, "");
            }
            return;
        }
        case OP_THREAD_JOIN:
            registers[CMT]=join_thread((int32_t )_64ADX);
            return;
        case OP_THREAD_INTERRUPT:
            registers[CMT]=interrupt_thread((int32_t )_64ADX);
            return;
        case OP_THREAD_CREATE:
            registers[EBX]=create_thread((int32_t )_64ADX, (bool)_64EBX);
            return;
        case OP_THREAD_PRIORITY:
            registers[CMT]=set_thread_priority((int32_t )_64ADX, (int)_64EGX);
            return;
        case OP_THREAD_SUSPEND:
            registers[CMT]=suspend_thread((int32_t )_64ADX);
            return;
        case OP_THREAD_UNSUSPEND:
            registers[CMT]=unsuspend_thread((int32_t )_64ADX, (int32_t )_64EBX);
            return;
        case OP_THREAD_CURRENT: // native getCurrentThread()
            grow_stack2(thread_self)
            THREAD_STACK_CHECK(thread_self);
            (++thread_self->this_fiber->sp)->object = thread_self->currentThread;
            return;
        case OP_THREAD_ARGS: // native getCurrentThreadArgs()
            grow_stack2(thread_self)
            THREAD_STACK_CHECK(thread_self);
            (++thread_self->this_fiber->sp)->object = thread_self->args;
            return;
        case OP_THREAD_SET_CURRENT: // native setCurrentThread(Thread)
            thread_self->currentThread = (thread_self->this_fiber->sp--)->object;
            return;
        case OP_MATH:
            _64CMT=__cmath(_64EBX, _64EGX, (int)_64ECX);
            return;
        case OP_RANDOM_INT:
            _64EBX = randInt();
            return;
        case OP_RANDOM_DOUBLE:
            _64EBX = randDouble();
            return;
        case OP_RANDOM_INT_RANGED:
            _64EBX = randInt((Int)_64EBX, (Int)_64EGX);
            return;
        case OP_RANDOM_DOUBLE_RANGED:
            _64EBX = randDouble(_64EBX, _64EGX);
            return;
        case OP_RANDOM_SEED:
            setSeed(_64ADX);
            return;
        case OP_SYSTEM_EXE: {
            SharpObject* str = (thread_self->this_fiber->sp--)->object.object;
            if(str != NULL && TYPE(str->info) == _stype_var && str->HEAD != NULL) {
                string cmd;
                populateString(cmd, str->HEAD, str->size);
                _64CMT= system(cmd.c_str());
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
#ifdef SHARP_PROF_
            thread_self->tprof->endtm=Clock::realTimeInNSecs();
            thread_self->tprof->profile();
            thread_self->tprof->dump();

#endif
            thread_self->this_fiber->exitVal = (int)(thread_self->this_fiber->sp--)->var;
            vm.shutdown();
            return;
        case OP_MEMORY_LIMIT:
            registers[CMT]=gc.getMemoryLimit();
            return;
        case OP_MEMORY:
            registers[CMT]=gc.getManagedMemory();
            return;
        case OP_ABS_PATH: {
            Object *relPath = &thread_self->this_fiber->sp->object;

            if(relPath->object != NULL && TYPE(relPath->object->info) == _stype_var && relPath->object->HEAD != NULL) {
                string path, absolute;
                populateString(path, relPath->object->HEAD, relPath->object->size);

                File::resolvePath(path, absolute);
                gc.createStringArray(relPath, absolute);
            } else {
                throw Exception(vm.NullptrExcept, "");
            }
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
            size_t len = (thread_self->this_fiber->sp--)->var;
            SharpObject *arry = thread_self->this_fiber->sp->object.object;

            if(arry != NULL) {
                if(len <= 0) {
                    stringstream ss;
                    ss << "invalid call to realloc() len: " << len
                       << ", size: " << arry->size;
                    throw Exception(ss.str());
                }

                if(TYPE(arry->info) == _stype_var) { // var[]
                    gc.realloc(arry, len);
                } else if(TYPE(arry->info) == _stype_struct && arry->node != NULL) { // object? maybe...
                    gc.reallocObject(arry, len);
                } else
                    throw Exception(vm.NullptrExcept, "");
            } else
                throw Exception(vm.NullptrExcept, "");
            return;
        }
        case OP_CURRENT_DIRECTORY: {
            string path;
            current_directory(path);
            gc.createStringArray(&(++thread_self->this_fiber->sp)->object, path);
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
            Object *arry = &(thread_self->this_fiber->sp--)->object;
            if(arry->object != NULL && arry->object->size > 0 && TYPE(arry->object->info)==_stype_var) {
                string path;
                populateString(path, arry->object->HEAD, arry->object->size);

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
                    _List<string> files;
                    get_file_list(path, files);
                    arry = &(++thread_self->this_fiber->sp)->object;

                    if(files.size()>0) {
                        *arry = gc.newObjectArray(files.size(), vm.StringClass);

                        for(long i = 0; i < files.size(); i++) {
                            arry->object->node[i] = gc.newObject(vm.StringClass);
                            gc.createStringArray(vm.resolveField("data", arry->object->node[i].object), files.get(i));
                            files.get(i).clear();
                        }
                    } else {
                        *arry = (SharpObject*)NULL;
                    }

                    files.free();
                }
                else if(signal==OP_CREATE_DIR)
                    registers[EBX] = File::makeDir(path);
                else if(signal==OP_DELETE_DIR)
                    registers[EBX] = delete_dir(path);
                else if(signal==OP_UPDATE_FILE_TM)
                    registers[EBX] = update_time(path, (time_t)registers[EBX]);
                else if(signal==OP_CHMOD)
                    registers[EBX] = __chmod(path, (mode_t)registers[EBX], (bool)registers[EGX], (bool)registers[ECX]);
                else if(signal==OP_READ_FILE) {
                    arry = &(++thread_self->this_fiber->sp)->object;
                    string str;
                    read_file(path, str);

                    if(str.size() > 0) {
                        *arry = gc.newObject(vm.StringClass);
                        gc.createStringArray(vm.resolveField("data", arry->object), str);
                    } else {
                        cout << "{{{{{{{ RELEASE!!" << endl;
                        gc.releaseObject(arry);
                    }
                }
            } else
                throw Exception(vm.NullptrExcept, "");
            return;
        }
        case OP_RENAME_FILE:
        case OP_WRITE_FILE:  {
            SharpObject *pathObj = (thread_self->this_fiber->sp--)->object.object;
            SharpObject *newNameObj = (thread_self->this_fiber->sp--)->object.object;

            if (pathObj != NULL && TYPE(pathObj->info) == _stype_var && newNameObj != NULL && TYPE(newNameObj->info) == _stype_var
                && pathObj->HEAD != NULL && newNameObj->HEAD != NULL) {
                string path, rename;

                populateString(path, pathObj->HEAD, pathObj->size);
                populateString(rename, newNameObj->HEAD, newNameObj->size);

                if(signal==OP_RENAME_FILE)
                    registers[EBX] = rename_file(path, rename);
                else if(signal==OP_WRITE_FILE) {
                    File::buffer buf;
                    buf.operator<<(rename); // rename will contain our actual unicode data
                    registers[EBX] = File::write(path.c_str(), buf);
                    buf.end();
                }
            } else {
                throw Exception(vm.NullptrExcept, "");
            }

            return;
        }
        case OP_DISK_SPACE:
            registers[EBX]=disk_space((int32_t )registers[EBX]);
            return;
        case OP_SIZEOF:
            registers[EBX] = GarbageCollector::_sizeof((thread_self->this_fiber->sp--)->object.object);
            return;
        case OP_FLUSH:
            cout << std::flush;
            break;
        case OP_GET_FRAME_INFO:
            grow_stack2(thread_self)
            THREAD_STACK_CHECK(thread_self);
            getFrameInfo(&(++thread_self->this_fiber->sp)->object);
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
        case OP_LOAD_LIBRARY: {
            SharpObject *libNameObj = (thread_self->this_fiber->sp--)->object.object;

            if (libNameObj != NULL && TYPE(libNameObj->info) == _stype_var && libNameObj->HEAD != NULL) {
                string name;

                populateString(name, libNameObj->HEAD, libNameObj->size);
                GUARD(library_mutex)

                if(vm.getLib(name) == NULL) {
                    Library lib;

                    #ifndef _WIN32
                    char *libError = dlerror(); // we need this variable to ensure everything is okay
                    #endif
                    lib.handle = load_lib(name);

#ifndef _WIN32
                    libError = dlerror();
#endif

                    if(!lib.handle

                    #ifndef _WIN32
                        || libError != NULL
                    #endif
                    ) {
                        string libName = name;
                        #ifdef _WIN32
                        throw Exception(vm.IllStateExcept, string("could not load library") + libName);
                        #else
                        throw Exception(vm.IllStateExcept, string("could not load library: ") + libError);
                        #endif
                    }

                    lib_handshake _lib_handshake =
                            (lib_handshake)load_func(lib.handle,
                                    "snb_handshake");

                    if(_lib_handshake && setupLibrary(_lib_handshake)) {
                        vm.libs.__new().init();
                        vm.libs.last().name = name;
                        vm.libs.last().handle = lib.handle;
                    } else {
                        throw Exception(vm.IllStateExcept, "handshake failed, could not load library");
                    }
                }
            } else {
                throw Exception(vm.NullptrExcept, "");
            }
            return;
        }
        case OP_FREE_LIBRARY: {
            SharpObject *libNameObj = (thread_self->this_fiber->sp--)->object.object;

            if (libNameObj != NULL && TYPE(libNameObj->info) == _stype_var && libNameObj->HEAD != NULL) {
                string name;
                populateString(name, libNameObj->HEAD, libNameObj->size);
                GUARD(library_mutex)
                registers[EBX] = vm.freeLib(name);
            }
            return;
        }
        case OP_LINK_FUNC: {
            SharpObject *libNameObj = (thread_self->this_fiber->sp--)->object.object;
            SharpObject *funcNameObj = (thread_self->this_fiber->sp--)->object.object;

            if (libNameObj != NULL && TYPE(libNameObj->info) == _stype_var && libNameObj->HEAD != NULL
              && funcNameObj != NULL && TYPE(funcNameObj->info) == _stype_var && funcNameObj->HEAD != NULL) {
                string libname, funcname;

                populateString(libname, libNameObj->HEAD, libNameObj->size);
                populateString(funcname, funcNameObj->HEAD, funcNameObj->size);
                GUARD(library_mutex)
                registers[EBX] = vm.link(funcname, libname);
            }
            return;
        }
        case OP_WAIT: {
            Object *obj = &(thread_self->this_fiber->sp--)->object;
            obj->wait();
            return;
        }
        case OP_NOTIFY: {
            Object *obj = &(thread_self->this_fiber->sp--)->object;
            obj->notify();
            return;
        }
        case OP_NOTIFY_FOR: {
            Object *obj = &(thread_self->this_fiber->sp--)->object;
            obj->notify((uInt)registers[EBX]);
            return;
        }
        case OP_EXPORT: {
            GUARD(exportMutex);
            string str(export_obj((thread_self->this_fiber->sp--)->object.object));
            gc.createStringArray(&(++thread_self->this_fiber->sp)->object, str);
            cleanup();
            return;
        }
        case OP_IMPORT: {
            GUARD(exportMutex);
            import_obj((thread_self->this_fiber->sp--)->object.object);
            cleanup();
            return;
        }
        default: {
            stringstream ss;
            ss << "invalid system interrupt signal: " << signal;
            throw Exception(vm.InvalidOperationExcept, ss.str());
        }
    }
}

bool VirtualMachine::catchException() { // todo: check if function has thread_self flag if so then release instance if non null and there is no catch
    Thread *thread = thread_self;
    Int pc = PC(thread);
    TryCatchData *tbl=NULL;
    ClassObject *handlingClass = &vm.classes[CLASS(thread->this_fiber->exceptionObject.object->info)];
    for(Int i = 0; i < thread->this_fiber->current->tryCatchTable.len; i++) {
        tbl = &thread->this_fiber->current->tryCatchTable._Data[i];

        if (pc >= tbl->block_start_pc && pc <= tbl->block_end_pc)
        {
            for(Int j = 0; j < tbl->catchTable.len; j++) {
                if(handlingClass->guid == tbl->catchTable._Data[j].caughtException->guid
                    || tbl->catchTable._Data[j].caughtException == vm.ExceptionClass
                    || tbl->catchTable._Data[j].caughtException == vm.ErrorClass
                    || tbl->catchTable._Data[j].caughtException == vm.Throwable) {
                    if(tbl->catchTable._Data[j].localFieldAddress >= 0)
                        (thread->this_fiber->fp+tbl->catchTable._Data[j].localFieldAddress)->object = thread->this_fiber->exceptionObject;
                    thread->this_fiber->pc = thread->this_fiber->cache+tbl->catchTable._Data[j].handler_pc;

                    thread->this_fiber->exceptionObject = (SharpObject*)NULL;
                    sendSignal(thread->signal, tsig_except, 0);
                    return true;
                }
            }
        }
    }

    // Since we couldn't catch it we must jump to the first finally block
    for(Int i = 0; i < thread->this_fiber->current->tryCatchTable.len; i++) {
        tbl = &thread->this_fiber->current->tryCatchTable._Data[i];

        if (tbl->try_start_pc <= pc && tbl->try_end_pc >= pc)
        {
            if(tbl->finallyData != NULL) {
                thread->this_fiber->pc = thread->this_fiber->cache+tbl->finallyData->start_pc;
                (thread->this_fiber->fp+tbl->finallyData->exception_object_field_address)->object = thread->this_fiber->exceptionObject;
                thread->this_fiber->exceptionObject = (SharpObject*)NULL;
                sendSignal(thread->signal, tsig_except, 0); // TODO: this may be causing an out of memory error (look to see if the ref is 1 after exception has been handled)
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
            fillStackTrace(methods, pcList, data);
        }
    }
}

void VirtualMachine::fillMethodCall(Method* func, Int pc, stringstream &ss) {
    if(func == NULL) return;

    ss << "\tSource ";
    if(func->sourceFile != -1 && func->sourceFile < vm.manifest.sourceFiles) {
        ss << "\""; ss << vm.metaData.files.get(func->sourceFile).name << "\"";
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

    ss << ", in "; ss << func->fullName << "(" << (func->nativeFunc ? "native" : "") << ")";

    if(c_options.debugMode) {
        ss << " ["
           << func->address << "] $" << pc;
        ss << " " << (func->nativeFunc ? "(jit)" : "");
    }

    if(line != -1 && vm.metaData.files.size() > 0) {
        ss << getPrettyErrorLine(line, func->sourceFile);
    }

    ss << "\n";
}

void VirtualMachine::fillStackTrace(Object *methods, Object *pcList, Object *data) {
    if(methods && pcList && methods->object && pcList->object && TYPE(methods->object->info) == _stype_var
        && TYPE(pcList->object->info) == _stype_var && data) {

        stringstream ss;
        for(Int i = 0; i < methods->object->size; i++) {
            fillMethodCall(&vm.methods[(Int)methods->object->HEAD[i] % vm.manifest.methods], (Int)pcList->object->HEAD[i], ss);
        }

        std::string str = ss.str(); ss.str("");
        gc.createStringArray(data, str);
        str.clear();
    }
}

void VirtualMachine::fillStackTrace(string &str) {
// fill message
    if(thread_self->this_fiber->callStack == NULL) return;

    stringstream ss;
    uInt iter = 0;
    if((thread_self->this_fiber->calls+1) < EXCEPTION_PRINT_MAX) {

        for(Int i = 1; i <= thread_self->this_fiber->calls; i++) {
            if(iter++ >= EXCEPTION_PRINT_MAX)
                break;
            fillMethodCall(&vm.methods[thread_self->this_fiber->callStack[i].returnAddress], thread_self->this_fiber->callStack[i].pc-thread_self->this_fiber->cache, ss);
        }
    } else {
        for(Int i = (thread_self->this_fiber->calls+1) - EXCEPTION_PRINT_MAX; i < thread_self->this_fiber->calls+1; i++) {
            if(iter++ >= EXCEPTION_PRINT_MAX)
                break;
            fillMethodCall(& vm.methods[thread_self->this_fiber->callStack[i].returnAddress], thread_self->this_fiber->callStack[i].pc-thread_self->this_fiber->cache, ss);
        }
    }

    fillMethodCall(thread_self->this_fiber->current, PC(thread_self), ss);
    str = ss.str(); ss.str("");
}

string VirtualMachine::getPrettyErrorLine(long line, long sourceFile) {
    stringstream ss;
    line -=2;

    if(line >= 0 && vm.metaData.hasLine(line, sourceFile)) {
        ss << endl << "\t   " << line << ":    ";
        ss << vm.metaData.getLine(line, sourceFile);
    }
    line++;

    if(line >= 0 && vm.metaData.hasLine(line, sourceFile)) {
        ss << endl << "\t   " << line << ":    ";
        ss << vm.metaData.getLine(line, sourceFile);
    }
    line++;

    if(line >= 0 && vm.metaData.hasLine(line, sourceFile)) {
        ss << endl << "\t>  " << line << ":    ";
        ss << vm.metaData.getLine(line, sourceFile);
    }
    line++;

    if(line >= 0 && vm.metaData.hasLine(line, sourceFile)) {
        ss << endl << "\t   " << line << ":    ";
        ss << vm.metaData.getLine(line, sourceFile);
    }
    line++;

    if(line >= 0 && vm.metaData.hasLine(line, sourceFile)) {
        ss << endl << "\t   " << line << ":    ";
        ss << vm.metaData.getLine(line, sourceFile);
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
            string str(to_string((Int)val));
            gc.createStringArray(&(++thread_self->this_fiber->sp)->object, str);
            return;
        }
        case 'L': {
            string str(to_string((uInt)val));
            gc.createStringArray(&(++thread_self->this_fiber->sp)->object, str);
            return;
        }
        default: {
            string str(to_string(val));
            gc.createStringArray(&(++thread_self->this_fiber->sp)->object, str);
            return;
        }
    }

    string str(buf);
    gc.createStringArray(&(++thread_self->this_fiber->sp)->object, str);
}

Method *VirtualMachine::getMainMethod() {
    return &methods[manifest.entryMethod];
}

ClassObject *VirtualMachine::resolveClass(std::string fullName) {
    for(uInt i = 0; i < vm.manifest.classes; i++) {
        if(vm.classes[i].fullName == fullName) {
            return &vm.classes[i];
        }
    }

    return nullptr;
}

Object *VirtualMachine::resolveField(std::string name, SharpObject *classObject) {
    if(classObject && IS_CLASS(classObject->info)) {
        ClassObject *representedClass = &vm.classes[CLASS(classObject->info)];
        for(Int i = 0; i < representedClass->totalFieldCount; i++) {
            Field &field = representedClass->fields[i];
            if(field.name == name) {
                int stat = isStaticObject(classObject);
                if(isStaticObject(classObject) == IS_STATIC(field.flags)) {
                    return &classObject->node[field.address];
                }
                else {
                    return nullptr;
                }
            }
        }
    }

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

void VirtualMachine::setFieldVar(std::string name, SharpObject *classObject, Int index, double value) {
    Object *field = vm.resolveField(std::move(name), classObject);
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

void VirtualMachine::initializeField(std::string name, SharpObject *classObject, ClassObject *klass) {
    Object *field = vm.resolveField(std::move(name), classObject);
    if(field != NULL) {
        *field = gc.newObject(klass);
    }
}

Library *VirtualMachine::getLib(std::string name) {
    for(long i = 0; i < libs.size(); i++) {
        if(libs.get(i).name == name)
            return &libs.get(i);
    }

    return NULL;
}

int VirtualMachine::freeLib(std::string name) {
    Library *lib = NULL;
    Int index = 0;
    GUARD(libsMutex)
    for(long i = 0; i < libs.size(); i++) {
        if(libs.get(i).name == name) {
            lib = &libs.get(i);
            index = i;
            break;
        }
    }


    if(lib != NULL) {
        bridgeFun _bridgeFun =
                (bridgeFun)load_func(lib->handle,
                                     "snb_main");
        for(Int i = 0; i < vm.manifest.methods; i++) {
            if(vm.methods[i].bridge == _bridgeFun) {
                vm.methods[i].bridge = NULL;
                vm.methods[i].linkAddr=-1;
            }
        }

        free_lib(lib->handle);

        libs.remove(index);
        return 0;
    }

    return 1;
}

void VirtualMachine::locateBridgeAndCross(Method *nativeFun) {
    fptr fun;
    linkProc _linkProc;
    uint32_t linkAddr = 0;

    for(Int i = 0; i < libs.size(); i++) {
        _linkProc =
                (linkProc)load_func(libs.get(i).handle,
                                   "snb_link_proc");

        if(_linkProc) {
            if((linkAddr = _linkProc(nativeFun->fullName.c_str())) >= 0) {
                nativeFun->bridge =
                        (bridgeFun)load_func(libs.get(i).handle,
                                           "snb_main");
                if (nativeFun->bridge) {
                    nativeFun->linkAddr = linkAddr;
                    setupMethodStack(nativeFun->address, thread_self, true);
                    nativeFun->bridge(nativeFun->linkAddr);
                    returnMethod(thread_self);
                }

                break;
            }
        }
    }

    if(!nativeFun->bridge)
        throw Exception(vm.UnsatisfiedLinkExcept, "");
}

bool VirtualMachine::link(string &func, string &libame) {
    Library *lib = getLib(libame);
    linkProc _linkProc;
    uint32_t linkAddr;

    if(lib != NULL) {
        for (Int i = 0; i < vm.manifest.methods; i++) {
            if (func == vm.methods[i].fullName) {
                if(!vm.methods[i].nativeFunc) return false;

                _linkProc =
                        (linkProc) load_func(lib->handle,
                                             "snb_link_proc");

                if (_linkProc && (linkAddr = _linkProc(vm.methods[i].fullName.c_str())) >= 0) {
                    vm.methods[i].linkAddr = linkAddr;
                    vm.methods[i].bridge =
                            (bridgeFun) load_func(lib->handle,
                                                  "snb_main");
                    return vm.methods[i].bridge != NULL;

                }

                break;
            }
        }
    }

    return false;
}

bool VirtualMachine::isType(Object *obj, int32_t type) { // todo: fix this to accept type properly
    if(obj && obj->object) {
        if(TYPE(obj->object->info) == _stype_var) {
            if (type == -1) { // _int8
                return obj->object->ntype == 0;
            } else if (type == -2) { // _int16
                return obj->object->ntype == 1;
            } else if (type == -3) { // _int32
                return obj->object->ntype == 2;
            } else if (type == -4) { // _int64
                return obj->object->ntype == 3;
            } else if (type == -5) { // _uint8
                return obj->object->ntype == 4;
            } else if (type == -6) { // _uint16
                return obj->object->ntype == 5;
            } else if (type == -7) { // _uint32
                return obj->object->ntype == 6;
            } else if (type == -8) { // _uint64
                return obj->object->ntype == 7;
            } else if (type == -9) { // var
                return obj->object->ntype == 8;
            }
        } else {
            ClassObject *base = &vm.classes[CLASS(obj->object->info)], *klass = &vm.classes[type];
            return base->isClassRelated(klass);
        }
    } else throw Exception(vm.NullptrExcept, "");
    return 0;
}