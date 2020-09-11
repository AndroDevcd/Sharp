//
// Created by BraxtonN on 2/12/2018.
//

#include <cmath>
#include "Thread.h"
#include "memory/GarbageCollector.h"
#include "Exe.h"
#include "VirtualMachine.h"
#include "Opcode.h"
#include "register.h"
#include "Manifest.h"
#include "symbols/Object.h"
#include "../util/time.h"
#include "jit/_BaseAssembler.h"
#include "jit/Jit.h"

#ifdef WIN32_
#include <conio.h>
#elif defined(POSIX_)
#include "termios.h"
#endif

int32_t Thread::tid = ILL_THREAD_ID;
uInt Thread::maxThreadId = 0;
thread_local Thread* thread_self = NULL;
_List<Thread*> Thread::threads;
size_t threadStackSize = STACK_SIZE, internalStackSize = INTERNAL_STACK_SIZE;
recursive_mutex Thread::threadsMonitor;
recursive_mutex Thread::threadsListMutex;

bool find_thread(void* id, Thread* thread) {
    return *((int32_t*)id) == thread->id;
}

/*
 * Local registers for the thread to use
 */
thread_local double* registers = NULL;

void Thread::Startup() {
    threads.init();

    Thread* main = (Thread*)malloc(
            sizeof(Thread)*1);
    main->Create("Main", vm.getMainMethod());
    main->priority = THREAD_PRIORITY_HIGH;
}

/**
 * API level thread create called from Sharp
 *
 * @param methodAddress
 * @param stack_size
 * @return
 */
int32_t Thread::Create(int32_t methodAddress, bool daemon) {
    int32_t threadId = generateId();
    registers[CMT] = false;
    if(methodAddress < 0 || methodAddress >= vm.manifest.methods)
        return RESULT_THREAD_CREATE_FAILED;

    if(threadId == ILL_THREAD_ID)
        return RESULT_NO_THREAD_ID;

    Method* method = &vm.methods[methodAddress];
    if(method->paramSize!=0)
        return RESULT_THREAD_CREATE_FAILED;

    Thread* thread = (Thread*)malloc(
            sizeof(Thread)*1);

    stringstream ss;
    ss << "Thread@" << threadId;
    thread->init(ss.str(), threadId, method, daemon);

    pushThread(thread);
    registers[CMT] = true;
    return threadId;
}

void Thread::Create(string name, Method* main) {
    Int threadId = generateId();
    if(threadId == ILL_THREAD_ID)
        return;

    init(name, threadId, main, false, true);
    pushThread(this);
}

void Thread::CreateDaemon(string name) {
    Int threadId = generateId();
    if(threadId == ILL_THREAD_ID)
        return;

    init(name, threadId, NULL, true);
    pushThread(this);
}

void Thread::pushThread(Thread *thread) {
    GUARD(Thread::threadsListMutex);
    threads.add(thread);
}

void Thread::popThread(Thread *thread) {
    GUARD(Thread::threadsListMutex);
    Int pos = threads.indexof(find_thread, &thread->id);
    if(pos >= 0)
       threads.remove(pos);
}

Thread *Thread::getThread(int32_t id) {
    GUARD(Thread::threadsListMutex);

    Thread *thread = NULL;
    int32_t pos = threads.indexof(find_thread, &id);
    if(pos >= 0)
       thread = threads.at(pos);

    return thread;
}

void Thread::suspendSelf() {
    thread_self->suspended = true;
    {
        GUARD(thread_self->mutex);
        sendSignal(thread_self->signal, tsig_suspend, 0);
    }

    /*
	 * We call wait upon suspend. This function will
	 * sleep the thread most of the time. unsuspendThread() or
	 * resumeAllThreads() should be called to revive thread.
	 */
    thread_self->waitForUnsuspend();
}

void Thread::waitForUnsuspend() {
    const long sMaxRetries = 128 * 1024;

    long spinCount = 0;
    long retryCount = 0;

    this->state = THREAD_SUSPENDED;

    while (this->suspended)
    {
        if (retryCount++ == sMaxRetries)
        {
            spinCount++;
            retryCount = 0;
#ifdef WIN32_
            Sleep(1);
#endif
#ifdef POSIX_
            usleep(1*POSIX_USEC_INTERVAL);
#endif
        } else if(this->state == THREAD_KILLED) {
            this->suspended = false;
            return;
        } else if(vm.state == VM_SHUTTING_DOWN) {
            this->suspended = false;
            sendSignal(signal, tsig_kill, 1);
            return;
        }
    }

    {
        GUARD(mutex);
        this->state = THREAD_RUNNING;
        sendSignal(signal, tsig_suspend, 0);
    }
}

int Thread::start(int32_t id, size_t stackSize) {
    Thread *thread = getThread(id);

    if (thread_self != NULL && (thread == NULL || thread->state == THREAD_STARTED))
        return RESULT_ILL_THREAD_START;

    if(thread->state == THREAD_RUNNING)
        return RESULT_THREAD_RUNNING;

    if(thread->terminated)
        return RESULT_THREAD_TERMINATED;

    if(stackSize && !validStackSize(stackSize))
        return RESULT_INVALID_STACK_SIZE;

    thread->stackSize = (stackSize != 0 ? stackSize : threadStackSize);
    thread->exited = false;
    thread->state = THREAD_STARTED;
#ifdef WIN32_
    thread->thread = CreateThread(
            NULL,                     // default security attributes
            (thread->stackSize),                      // use default stack size
            &VirtualMachine::InterpreterThreadStart,  // thread function caller
            thread,                                   // thread self when thread is created
            0,                         // use default creation flags
            NULL);
    if(thread->thread == NULL) return RESULT_THREAD_NOT_STARTED; // thread was not started
    else return waitForThread(thread);
#endif
#ifdef POSIX_
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr,thread->stackSize + STACK_OVERFLOW_BUF);
    if(pthread_create( &thread->thread, &attr, VirtualMachine::InterpreterThreadStart, (void*) thread))
        return RESULT_THREAD_NOT_STARTED; // thread was not started
    else {
        return waitForThread(thread);
    }
#endif

}

int Thread::suspendThread(int32_t id) {
    Thread *thread = getThread(id);

    if (thread_self != NULL && (thread == NULL || thread->state != THREAD_RUNNING))
        return RESULT_ILL_THREAD_SUSPEND;

    if(thread->terminated)
        return RESULT_THREAD_TERMINATED;

    suspendThread(thread);
    return RESULT_OK;
}

void Thread::suspendFor(Int mills) {
    Thread *thread = thread_self;
    thread->suspended = true;
    {
        GUARD(thread->mutex)
        sendSignal(thread->signal, tsig_suspend, 0);
    }

    thread->waitForUnsuspend();
}

int Thread::unSuspendThread(int32_t id, bool wait) {
    Thread *thread = getThread(id);

    if (thread_self != NULL && (thread == NULL || (!wait && thread->state != THREAD_SUSPENDED)))
        return RESULT_ILL_THREAD_SUSPEND;

    if(thread->terminated)
        return RESULT_THREAD_TERMINATED;

    if(wait) {
        unsuspendAndWait(thread);
        return RESULT_OK;
    }

    return unsuspendThread(thread);
}

int Thread::destroy(int32_t id) {
    if(id == thread_self->id || id==main_threadid)
        return RESULT_ILL_THREAD_DESTROY; // cannot destroy self or main

    Thread* thread = getThread(id);
    if(thread == NULL || thread->daemon)
        return RESULT_ILL_THREAD_DESTROY;

    return destroy(thread);
}

int Thread::interrupt(int32_t id) {
    if(id == thread_self->id)
        return RESULT_ILL_THREAD_INTERRUPT; // cannot interrupt thread_self

    Thread* thread = getThread(id);
    if(thread == NULL || thread->daemon)
        return RESULT_THREAD_INTERRUPT_FAILED;
    if(thread->terminated)
        return RESULT_THREAD_TERMINATED;

    int result = interrupt(thread);
    if(result == RESULT_OK && vm.state != VM_TERMINATED)
        waitForThreadExit(thread);
    return result;
}

void Thread::suspendAndWait(Thread *thread) {
    suspendThread(thread);
    waitForThreadSuspend(thread);
}

void Thread::unsuspendAndWait(Thread *thread) {
    unsuspendThread(thread);
    waitForThreadUnSuspend(thread);
}

void Thread::waitForThreadSuspend(Thread *thread) {
    const int sMaxRetries = 10000000;
    const int sMaxSpinCount = 10; // TODO: test this extensivley to make sure there is no issues with lowering the threshold to giving up

    int spinCount = 0;
    int retryCount = 0;

    while (thread->state == THREAD_RUNNING && !thread->suspended)
    {
        if(thread->contextSwitching)
            return;

        if (retryCount++ == sMaxRetries)
        {
            suspendThread(thread);
            retryCount = 0;
            if(++spinCount >= sMaxSpinCount || thread->state >= THREAD_SUSPENDED)
                return;

#ifdef WIN32_
            Sleep(1);
#endif
#ifdef POSIX_
            usleep(1*POSIX_USEC_INTERVAL);
#endif
        }
    }
}

void Thread::waitForThreadUnSuspend(Thread *thread) {
    const int sMaxRetries = 10000000;
    const int sMaxSpinCount = 10;

    int spinCount = 0;
    int retryCount = 0;
    Thread *current = thread_self;

    while (thread->state != THREAD_RUNNING)
    {
        if (retryCount++ == sMaxRetries)
        {
            unsuspendThread(thread);
            retryCount = 0;
            if(++spinCount >= sMaxSpinCount)
                return;

#ifdef WIN32_
            Sleep(1);
#endif
#ifdef POSIX_
            usleep(1*POSIX_USEC_INTERVAL);
#endif
        }

        // to protect against de-sync issues caused by the gc
        if(hasSignal(current->signal, tsig_suspend) && vm.state != VM_SHUTTING_DOWN) {
            suspendSelf();
        }
    }
}

void Thread::waitForThreadExit(Thread *thread) {
    const int sMaxRetries = 10000000;
    int retryCount = 0;

    while (!thread->exited)
    {
        if (retryCount++ == sMaxRetries)
        {
            retryCount = 0;
            if(thread->exited)
                return;
#ifdef WIN32_
            Sleep(1);
#endif
#ifdef POSIX_
            usleep(1*POSIX_USEC_INTERVAL);
#endif
        }
    }
}

void Thread::terminateAndWaitForThreadExit(Thread *thread) {
    const int sMaxRetries = 10000000;
    const int sMaxSpinCount = 25;

    int spinCount = 0;
    int retryCount = 0;

    retry:
    {
        GUARD(thread->mutex);
        sendSignal(thread->signal, tsig_kill, 1);
    }

    while (!thread->exited)
    {
        if (retryCount++ == sMaxRetries)
        {
            retryCount = 0;
            if(thread->exited)
                return;
            else if(++spinCount >= sMaxSpinCount)
            {
                return;
            } else goto retry;

#ifdef WIN32_
            Sleep(1);
#endif
#ifdef POSIX_
            usleep(1*POSIX_USEC_INTERVAL);
#endif
        }
    }
}

/**
 * This function forces the current thread to wait for the thread to offically start,
 * however it is not garunteed to to wait for the thread if there was a problem with starting it.
 *
 * In theory this function depending on CPU clock speeds and other factors should not wait
 * longer than 50ms for any given thread to be started.
 *
 * @param thread
 * @return
 */
int Thread::waitForThread(Thread *thread) {
    const int sMaxRetries = 10000000;
    const int sMaxSpinCount = 25;

    int spinCount = 0;
    int retryCount = 0;

    while (thread->state != THREAD_RUNNING)
    {
        if (retryCount++ == sMaxRetries)
        {
            retryCount = 0;
            if(++spinCount >= sMaxSpinCount)
            {
                return RESULT_MAX_SPIN_GIVEUP;
            }

#ifdef WIN32_
            Sleep(1);
#endif
#ifdef POSIX_
            usleep(1*POSIX_USEC_INTERVAL);
#endif
        }
    }

    return RESULT_OK;
}

void Thread::suspendAllThreads() {
    Thread* thread;

    for(uInt i = 0; i < threads.size(); i++) {
        thread = threads.at(i);

        if(thread->id != thread_self->id){
            if(thread->state == THREAD_RUNNING)
                suspendAndWait(thread);
        }
    }
}

void Thread::resumeAllThreads() {
    Thread* thread;

    for(uInt i = 0; i < threads.size(); i++) {
        thread = threads.at(i);

        if(thread->id != thread_self->id){
            unsuspendAndWait(thread);
        }
    }
}

int Thread::unsuspendThread(Thread *thread) {
    GUARD(thread->mutex);
    thread->suspended = false;
    sendSignal(thread->signal, tsig_suspend, 0);
    return RESULT_OK;
}

void Thread::suspendThread(Thread *thread) {
    if(thread->id == thread_self->id)
        suspendSelf();
    else {
        GUARD(thread->mutex);
        sendSignal(thread->signal, tsig_suspend, 1);
    }
}

void Thread::term() {
    GUARD(mutex);
    if(this_fiber) this_fiber->bind(NULL);
    sendSignal(this->signal, tsig_kill, 1);
    this->terminated = true;

#ifdef SHARP_PROF_
    tprof->free();
    delete tprof;
#endif
    this->name.free();

    if(this_fiber) {
        this_fiber->setState(this, FIB_KILLED);
    }
    SharpObject *nill = NULL;
    currentThread = nill;
    args = nill;
    this->state = THREAD_KILLED;
#ifdef BUILD_JIT
    if(jctx != NULL) {
        delete jctx; jctx = NULL;
    }
#endif
}

int Thread::join(int32_t id) {
    if (thread_self != NULL && (id == thread_self->id ))//|| id <= jit_threadid)) // TODO: update when jit is active
        return RESULT_ILL_THREAD_JOIN;

    Thread* thread = getThread(id);
    if (thread == NULL)
        return RESULT_ILL_THREAD_JOIN;
    if(thread->terminated)
        return RESULT_THREAD_TERMINATED;

    return threadjoin(thread);
}

int Thread::threadjoin(Thread *thread) {
    if(thread->state == THREAD_STARTED)
        waitForThread(thread);

    if (thread->state == THREAD_RUNNING
        || thread->state == THREAD_SUSPENDED)
    {
#ifdef WIN32_
        WaitForSingleObject(thread->thread, INFINITE);
        return RESULT_OK;
#endif
#ifdef POSIX_
        if(pthread_join(thread->thread, NULL) != 0)
            return RESULT_THREAD_JOIN_FAILED;
        else return RESULT_OK;
#endif
    }

    return RESULT_THREAD_JOIN_FAILED;
}

void Thread::killAll() {
    GUARD(threadsMonitor);
    suspendAllThreads();
    Thread *thread = NULL;

    for(uInt i = 0; i < threads.size(); i++) {
        thread = threads.at(i);

        if(thread->id != thread_self->id
           && thread->state != THREAD_KILLED && thread->state != THREAD_CREATED) {
            terminateAndWaitForThreadExit(thread);
        }
    }
}

/**
 * We must use sendSignal() synanomyously with interrupt and other calls
 * that may kill a thread because this bit flag is used by the JIT Runtime
 * @param thread
 * @return
 */
int Thread::interrupt(Thread *thread) {
    if (thread->state == THREAD_RUNNING)
    {
        if (thread->id == main_threadid)
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
        else
        {
            GUARD(thread->mutex);
            sendSignal(thread->signal, tsig_kill, 1);
        }

        return RESULT_OK;
    }

    return RESULT_THREAD_INTERRUPT_FAILED;
}

void Thread::shutdown() {
    if(tid != ILL_THREAD_ID) {
        Thread::killAll();
        Thread* thread;

        for(uInt i = 0; i < threads.size(); i++) {
            thread = threads.at(i);
            std::free(thread);
        }
        threads.free();
    }
}

void Thread::exit() {
    GUARD(mutex);
    if(this_fiber) this_fiber->bind(NULL);
    if(id == main_threadid) {
        if (this_fiber && this_fiber->dataStack != NULL)
            this_fiber->exitVal = (int) this_fiber->dataStack[vm.manifest.threadLocals].var;
    }

    printException();
    releaseResources();
    if(this_fiber) {
        this_fiber->setState(this, FIB_KILLED);
    }
    this->signal = tsig_empty;
    this->exited = true;
    this->state = THREAD_KILLED;
}

void Thread::printException() {
    GUARD(mutex)
    if(hasSignal(signal, tsig_except) && this_fiber) {
        Object* frameInfo = vm.resolveField("frame_info", this_fiber->exceptionObject.object);
        Object* message = vm.resolveField("message", this_fiber->exceptionObject.object);
        Object* stackTrace = vm.resolveField("stack_trace", this_fiber->exceptionObject.object);
        ClassObject *exceptionClass = NULL;
        if(this_fiber->exceptionObject.object != NULL)
            exceptionClass = &vm.classes[CLASS(this_fiber->exceptionObject.object->info)];


        cout << "Unhandled exception on thread " << name.str() << " (most recent call last):\n";
        if(stackTrace != NULL && stackTrace->object != NULL){
            Object* data = vm.resolveField("data", stackTrace->object);

            if(data != NULL) {
                cout << vm.stringValue(data->object);
            }
        } else if(frameInfo && frameInfo->object) {
            if(((this_fiber->sp - this_fiber->dataStack) + 1) >= this_fiber->stackLimit) {
                this_fiber->sp = this_fiber->dataStack - 1;
            }

            if(exceptionClass != NULL && exceptionClass->guid != vm.OutOfMemoryExcept->guid) {
                (++this_fiber->sp)->object = frameInfo;
                vm.getStackTrace();
                Object* data = vm.resolveField("data", this_fiber->sp->object.object);

                if(data != NULL) {
                    cout << vm.stringValue(data->object);
                }
            }
        }

        sendSignal(signal, tsig_except, 0);
        cout << endl << (exceptionClass != NULL ? exceptionClass->name.str() : "") << " ("
           << (message != NULL ? vm.stringValue(message->object) : "") << ")\n";
    }
}

void Thread::releaseResources() {
    gc.reconcileLocks(this);
}

int Thread::startDaemon(
#ifdef WIN32_
        DWORD WINAPI
#endif
#ifdef POSIX_
void*
#endif
(*threadFunc)(void *), Thread *thread) {
    if (thread == NULL || !thread->daemon)
        return RESULT_ILL_THREAD_START;

    if(thread->state == THREAD_RUNNING)
        return RESULT_THREAD_RUNNING;

    thread->exited = false;
#ifdef WIN32_
    thread->thread = CreateThread(
            NULL,                   // default security attributes
            0,                      // use default stack size
            (LPTHREAD_START_ROUTINE)threadFunc,       // thread function caller
            thread,                 // thread self when thread is created
            0,                      // use default creation flags
            NULL);
    if(thread->thread == NULL) return RESULT_THREAD_NOT_STARTED; // thread was not started
    else
        return waitForThread(thread);
#endif
#ifdef POSIX_
    if(pthread_create( &thread->thread, NULL, threadFunc, (void*) thread)!=0)
        return RESULT_THREAD_NOT_STARTED; // thread was not started
    else
        return waitForThread(thread);
#endif
}

#ifdef DEBUGGING
void printRegs() {
    cout << endl;
    if(registers != NULL) {
        cout << "Registers: \n";
        cout << "adx = " << registers[ADX] << endl;
        cout << "cx = " << registers[CX] << endl;
        cout << "cmt = " << registers[CMT] << endl;
        cout << "ebx = " << registers[EBX] << endl;
        cout << "ecx = " << registers[ECX] << endl;
        cout << "ecf = " << registers[ECF] << endl;
        cout << "edf = " << registers[EDF] << endl;
        cout << "ehf = " << registers[EHF] << endl;
        cout << "bmr = " << registers[BMR] << endl;
        cout << "egx = " << registers[EGX] << endl;
    }

    if(thread_self && thread_self->this_fiber) {
        cout << "sp -> " << (thread_self->this_fiber->sp - thread_self->this_fiber->dataStack) << endl;
        cout << "fp -> " << (thread_self->this_fiber->fp - thread_self->this_fiber->dataStack) << endl;
        cout << "pc -> " << PC(thread_self->this_fiber) << endl;


        native_string stackTrace;
        vm.fillStackTrace(stackTrace);
        cout << "call stack (most recent call last):\n" << stackTrace.str() << endl;
        if (thread_self->this_fiber->current != NULL) {
            cout << "current function -> " << thread_self->this_fiber->current->fullName.str() << endl;
        }

        if (thread_self->this_fiber->dataStack) {
            for (long i = 0; i < 15; i++) {
                cout << "fp.var [" << i << "] = " << thread_self->this_fiber->dataStack[i].var << ";" << endl;
            }
        }
    }
}

void printStack() {
    native_string str;
    vm.fillStackTrace(str);
    cout << " stack\n" << str.str() << endl << std::flush;
}
#endif


string dataTypeToString(DataType varType, bool isArray) {
    string type;
    switch(varType) {
        case _UINT8:
            type = "_uint8";
            break;
        case _UINT16:
            type = "_uint16";
            break;
        case _UINT32:
            type = "_uint32";
            break;
        case _UINT64:
            type = "_uint64";
            break;
        case _INT8:
            type = "_int8";
            break;
        case _INT16:
            type = "_int16";
            break;
        case _INT32:
            type = "_int32";
            break;
        case _INT64:
            type = "_int64";
            break;
        case VAR:
            type = "var";
            break;
    }
    stringstream ss;
    ss << type << (isArray ? "[]" : "");
    return ss.str();
}

string getVarCastExceptionMsg(DataType varType, bool isArray, SharpObject *obj) {

    stringstream ss;
    ss << "illegal cast to `"<< dataTypeToString(varType, isArray) << "` from type `" << dataTypeToString(obj->ntype == 8 ? VAR : (DataType)obj->ntype, obj->size > 1) << "`";
    return ss.str();
}


/**
 * TODO: update exception system
 * I will keep the finally table but add an instruction to the VM for checking if we are in an "exception state" and
 * if so jump to the next respective finally block and then return if the last finally block in the function has been hit
 * we return from the function and keep doing that until we bounce out of the main function
 *
 * This will make the exception system very fast as it will only rely on the "initial firing" of the exception to figure out where it needs to jump
 * and the compiler will do the rest of the hard work by just running code
 *
 * also for the throw instruction instead of throwing the exception simply just call the respective function for the intitial firing of the exception
 * any other code that throws a exception while maybe calling a system interrupt or something will be caught and call the same thing
 *
 */
unsigned long irCount = 0, overflow = 0;

void Thread::exec() {

    registers = this_fiber->registers;
    Object *tmpPtr;
    SharpObject* tmpShObj;
    Int result;
    fptr jitFun;
    HAS_SIGNAL

#ifdef SHARP_PROF_
    tprof->init(stackLimit);
    tprof->starttm=Clock::realTimeInNSecs();
    for(size_t i = 0; i < vm.manifest.methods; i++) {
        funcProf prof = funcProf(vm.methods+i);
        tprof->functions.push_back(prof);
    }
#endif

#ifdef SHARP_PROF_
    tprof->hit(main);
#endif

    _initOpcodeTable
    run:
    try {
        for (;;) {
            top:
                if(this_fiber->current->address == 2133 && (PC(this_fiber) >= 71)) { // tutoriall!!!!!!!!!!!!!!!!!!
                    Int i = 0;
                }
                DISPATCH();
            _NOP: // tested
                _brh
            _INT:

#ifdef SHARP_PROF_
            if(GET_Da(*pc) == OP_EXIT) {
                tprof->endtm=Clock::realTimeInNSecs();
                tprof->profile();
                tprof->dump();
            }

#endif
                VirtualMachine::sysInterrupt(GET_Da(*this_fiber->pc));
                if(vm.state == VM_TERMINATED) return;

            context_switch_check(true)
            _brh
            _MOVI: // tested
                registers[GET_Da(*this_fiber->pc)]=(int32_t)*(this_fiber->pc+1);
                _brh_inc(2)
            RET: // tested
                result = GET_Da(*this_fiber->pc); // error state
                if(result == ERR_STATE && this_fiber->exceptionObject.object == NULL) {
                    this_fiber->exceptionObject
                            = (this_fiber->sp--)->object.object;
                }

                if(returnMethod(this)) {
                    // handle the exception even if we hit the last method or return back to JIT
                    if(result == ERR_STATE) {
                        GUARD(mutex);
                        sendSignal(signal, tsig_except, 1);
                    }
                    return;
                }

                if(result == ERR_STATE) {
                    GUARD(mutex);
                    sendSignal(signal, tsig_except, 1);
                    goto exception_catch;
                }

                context_switch_check(true)
                LONG_CALL();
                _brh
            HLT: // tested
                sendSignal(signal, tsig_kill, 1);
                _brh
            NEWARRAY: // tested
                STACK_CHECK
                (++this_fiber->sp)->object =
                        gc.newObject(registers[GET_Ca(*this_fiber->pc)], GET_Cb(*this_fiber->pc));
                _brh
            CAST:
                CHECK_NULL(this_fiber->ptr->castObject(registers[GET_Da(*this_fiber->pc)]);)
                _brh
            VARCAST:
                CHECK_NULL2(
                        result = GET_Ca(*this_fiber->pc) < FNPTR ? GET_Ca(*this_fiber->pc) : NTYPE_VAR; // ntype
                        if(!(TYPE(this_fiber->ptr->object->info) == _stype_var && this_fiber->ptr->object->ntype == result)) {
                            throw Exception(vm.ClassCastExcept,
                                    getVarCastExceptionMsg((DataType)GET_Ca(*this_fiber->pc), GET_Cb(*this_fiber->pc), this_fiber->ptr->object));
                        }
                )
                _brh
            MOV8: // tested
                registers[GET_Ca(*this_fiber->pc)]=(int8_t)registers[GET_Cb(*this_fiber->pc)];
                _brh
            MOV16: // tested
                registers[GET_Ca(*this_fiber->pc)]=(int16_t)registers[GET_Cb(*this_fiber->pc)];
                _brh
            MOV32: // tested
                registers[GET_Ca(*this_fiber->pc)]=(int32_t)registers[GET_Cb(*this_fiber->pc)];
                _brh
            MOV64: // tested
                registers[GET_Ca(*this_fiber->pc)]=(Int)registers[GET_Cb(*this_fiber->pc)];
                _brh
            MOVU8: // tested
                registers[GET_Ca(*this_fiber->pc)]=(uint8_t)registers[GET_Cb(*this_fiber->pc)];
                _brh
            MOVU16: // tested
                registers[GET_Ca(*this_fiber->pc)]=(uint16_t)registers[GET_Cb(*this_fiber->pc)];
                _brh
            MOVU32: // tested
                registers[GET_Ca(*this_fiber->pc)]=(uint32_t)registers[GET_Cb(*this_fiber->pc)];
                _brh
            MOVU64: // tested
                registers[GET_Ca(*this_fiber->pc)]=(uInt)registers[GET_Cb(*this_fiber->pc)];
                _brh
            RSTORE: // tested
                STACK_CHECK
                (++this_fiber->sp)->var = registers[GET_Da(*this_fiber->pc)];
                 _brh
            ADD: // tested
                registers[GET_Bc(*this_fiber->pc)]=registers[GET_Ba(*this_fiber->pc)]+registers[GET_Bb(*this_fiber->pc)];
                _brh
            SUB: // tested
                registers[GET_Bc(*this_fiber->pc)]=registers[GET_Ba(*this_fiber->pc)]-registers[GET_Bb(*this_fiber->pc)];
                _brh
            MUL: // tested
                registers[GET_Bc(*this_fiber->pc)]=registers[GET_Ba(*this_fiber->pc)]*registers[GET_Bb(*this_fiber->pc)];
                _brh
            DIV: // tested
                if(registers[GET_Ba(*this_fiber->pc)]==0 && registers[GET_Bb(*this_fiber->pc)]==0) throw Exception("divide by 0");
                registers[GET_Bc(*this_fiber->pc)]=registers[GET_Ba(*this_fiber->pc)]/registers[GET_Bb(*this_fiber->pc)];
                _brh
            MOD: // tested
                registers[GET_Bc(*this_fiber->pc)]=(Int)registers[GET_Ba(*this_fiber->pc)]%(Int)registers[GET_Bb(*this_fiber->pc)];
                _brh
            IADD: // tested
                registers[GET_Da(*this_fiber->pc)]+=(int32_t)*(this_fiber->pc+1);
                _brh_inc(2)
            ISUB: // tested
                registers[GET_Da(*this_fiber->pc)]-=(int32_t)*(this_fiber->pc+1);
                _brh_inc(2)
            IMUL: // tested
                registers[GET_Da(*this_fiber->pc)]*=(int32_t)*(this_fiber->pc+1);
                _brh_inc(2)
            IDIV: // tested
                if(((int32_t)*(this_fiber->pc+1))==0) throw Exception("divide by 0");
                registers[GET_Da(*this_fiber->pc)]/=(int32_t)*(this_fiber->pc+1);
                _brh_inc(2)
            IMOD: // tested
                registers[GET_Da(*this_fiber->pc)]=(Int)registers[GET_Da(*this_fiber->pc)]%(Int)*(this_fiber->pc+1);
                _brh_inc(2)
            POP: // tested
                this_fiber->sp->object = (SharpObject*)NULL;
                --this_fiber->sp;
                _brh
            INC: // tested
                registers[GET_Da(*this_fiber->pc)]++;
                _brh
            DEC: // tested
                registers[GET_Da(*this_fiber->pc)]--;
                _brh
            MOVR: // tested
                registers[GET_Ca(*this_fiber->pc)]=registers[GET_Cb(*this_fiber->pc)];
                _brh
            BRH: // tested
                HAS_SIGNAL
                this_fiber->pc=this_fiber->cache+(Int)registers[ADX];
                LONG_CALL();
                context_switch_check(false)
                _brh_NOINCREMENT
            IFE:
                LONG_CALL();
                HAS_SIGNAL
                if(registers[CMT]) {
                    this_fiber->pc=this_fiber->cache+(int64_t)registers[ADX];
                    context_switch_check(false)
                    _brh_NOINCREMENT
                } else  _brh
            IFNE:
                LONG_CALL();
                HAS_SIGNAL
                if(registers[CMT]==0) {
                    this_fiber->pc=this_fiber->cache+(int64_t)registers[ADX];
                    context_switch_check(false)
                    _brh_NOINCREMENT
                } else  _brh
            LT:
                registers[CMT]=registers[GET_Ca(*this_fiber->pc)]<registers[GET_Cb(*this_fiber->pc)];
                _brh
            GT:
                registers[CMT]=registers[GET_Ca(*this_fiber->pc)]>registers[GET_Cb(*this_fiber->pc)];
                _brh
            LTE:
                registers[CMT]=registers[GET_Ca(*this_fiber->pc)]<=registers[GET_Cb(*this_fiber->pc)];
                _brh
            GTE:
                registers[CMT]=registers[GET_Ca(*this_fiber->pc)]>=registers[GET_Cb(*this_fiber->pc)];
                _brh
            MOVL: // tested
                this_fiber->ptr = &(this_fiber->fp+GET_Da(*this_fiber->pc))->object;
                _brh
            POPL: // tested
                (this_fiber->fp+GET_Da(*this_fiber->pc))->object
                        = (this_fiber->sp--)->object.object;
                _brh
            IPOPL: // tested
                (this_fiber->fp+GET_Da(*this_fiber->pc))->var
                        = (this_fiber->sp--)->var;
                _brh
            MOVSL: // tested
                this_fiber->ptr = &((this_fiber->sp+GET_Da(*this_fiber->pc))->object);
                _brh
            SIZEOF:
                if(this_fiber->ptr==NULL || this_fiber->ptr->object == NULL)
                    registers[GET_Da(*this_fiber->pc)] = 0;
                else
                    registers[GET_Da(*this_fiber->pc)]=this_fiber->ptr->object->size;
                _brh
            PUT: // tested
                cout << registers[GET_Da(*this_fiber->pc)];
                _brh
            PUTC:
                printf("%c", (char)registers[GET_Da(*this_fiber->pc)]);
                _brh
            GET:
                if(_64CMT)
                    registers[GET_Da(*this_fiber->pc)] = getche();
                else
                    registers[GET_Da(*this_fiber->pc)] = getch();
                _brh
            CHECKLEN:
                CHECK_NULL2(
                        if((registers[GET_Da(*this_fiber->pc)]<this_fiber->ptr->object->size) &&!(registers[GET_Da(*this_fiber->pc)]<0)) { _brh }
                        else {
                            stringstream ss;
                            ss << "Access to Object at: " << registers[GET_Da(*this_fiber->pc)] << " size is " << this_fiber->ptr->object->size;
                            throw Exception(vm.IndexOutOfBoundsExcept, ss.str());
                        }
                )
            JMP: // tested
                HAS_SIGNAL
                this_fiber->pc = this_fiber->cache+GET_Da(*this_fiber->pc);
                LONG_CALL();
                context_switch_check(false)
                _brh_NOINCREMENT
            LOADPC: // tested
                registers[GET_Da(*this_fiber->pc)] = PC(this_fiber);
                _brh
            PUSHOBJ:
                STACK_CHECK
                (++this_fiber->sp)->object = this_fiber->ptr;
                _brh
            DEL:
                gc.releaseObject(this_fiber->ptr);
                _brh
            CALL:
#ifdef SHARP_PROF_
            tprof->hit(vm.methods+GET_Da(*this_fiber->pc));
#endif
                if((jitFun = executeMethod(GET_Da(*this_fiber->pc), this)) != NULL) {

#ifdef BUILD_JIT
                    jitFun(jctx);
#endif
                }

                context_switch_check(false)
                _brh_NOINCREMENT
            CALLD:
#ifdef SHARP_PROF_
            tprof->hit(vm.methods+GET_Da(*this_fiber->pc));
#endif
                if((result = (int64_t )registers[GET_Da(*this_fiber->pc)]) <= 0 || result >= vm.manifest.methods) {
                    stringstream ss;
                    ss << "invalid call to method with address of " << result;
                    throw Exception(ss.str());
                }
                if((jitFun = executeMethod(result, this)) != NULL) {

#ifdef BUILD_JIT
                    jitFun(jctx);
#endif
                }

                context_switch_check(false)
                _brh_NOINCREMENT
            NEWCLASS:
                STACK_CHECK
                (++this_fiber->sp)->object =
                        gc.newObject(&vm.classes[*(this_fiber->pc+1)]);
                _brh_inc(2)
            MOVN:
                CHECK_NULLOBJ(
                        if(((int32_t)*(this_fiber->pc+1)) >= this_fiber->ptr->object->size || ((int32_t)*(this_fiber->pc+1)) < 0)
                            throw Exception("movn");

                        this_fiber->ptr = &this_fiber->ptr->object->node[((int32_t)*(this_fiber->pc+1))];
                )
                _brh_inc(2)
            SLEEP:
                __os_sleep((Int)registers[GET_Da(*this_fiber->pc)]);
                _brh
            TEST:
                registers[CMT]=registers[GET_Ca(*this_fiber->pc)]==registers[GET_Cb(*this_fiber->pc)];
                _brh
            TNE:
                registers[CMT]=registers[GET_Ca(*this_fiber->pc)]!=registers[GET_Cb(*this_fiber->pc)];
                _brh
            LOCK:
                CHECK_NULL2(
                        if(!Object::monitorLock(this_fiber->ptr, this)) {
                            return;
                        }
                )
                _brh
            ULOCK:
                CHECK_NULL2(Object::monitorUnLock(this_fiber->ptr);)
                _brh
            MOVG:
                this_fiber->ptr = vm.staticHeap+GET_Da(*this_fiber->pc);
                _brh
            MOVND:
                CHECK_NULLOBJ(
                        if(((int32_t)registers[GET_Da(*this_fiber->pc)]) >= this_fiber->ptr->object->size || ((int32_t)registers[GET_Da(*this_fiber->pc)]) < 0)
                            throw Exception("movn");

                        this_fiber->ptr = &this_fiber->ptr->object->node[(Int)registers[GET_Da(*this_fiber->pc)]];
                )
                _brh
            NEWOBJARRAY:
                (++this_fiber->sp)->object = gc.newObjectArray(registers[GET_Da(*this_fiber->pc)]);
                STACK_CHECK _brh
            NOT:
                registers[GET_Ca(*this_fiber->pc)]=!registers[GET_Cb(*this_fiber->pc)];
                _brh
            SKIP:
                HAS_SIGNAL
                this_fiber->pc = this_fiber->pc+GET_Da(*this_fiber->pc);
                _brh
            LOADVAL:
                registers[GET_Da(*this_fiber->pc)]=(this_fiber->sp--)->var;
                _brh
            SHL:
                registers[GET_Bc(*this_fiber->pc)]=(int64_t)registers[GET_Ba(*this_fiber->pc)]<<(int64_t)registers[GET_Bb(*this_fiber->pc)];
                _brh
            SHR:
                registers[GET_Bc(*this_fiber->pc)]=(int64_t)registers[GET_Ba(*this_fiber->pc)]>>(int64_t)registers[GET_Bb(*this_fiber->pc)];
                _brh
            SKPE:
                LONG_CALL();
                HAS_SIGNAL
                if(((Int)registers[GET_Ca(*this_fiber->pc)]) != 0) {
                    this_fiber->pc = this_fiber->pc+GET_Cb(*this_fiber->pc); _brh_NOINCREMENT
                } else _brh
            SKNE:
                LONG_CALL();
                HAS_SIGNAL
                if(((Int)registers[GET_Ca(*this_fiber->pc)])==0) {
                    this_fiber->pc = this_fiber->pc+GET_Cb(*this_fiber->pc); _brh_NOINCREMENT
                } else _brh
            CMP:
                registers[CMT]=registers[GET_Da(*this_fiber->pc)]==((int32_t)*(this_fiber->pc+1));
                _brh_inc(2)
            AND:
                registers[CMT]=registers[GET_Ca(*this_fiber->pc)]&&registers[GET_Cb(*this_fiber->pc)];
                _brh
            UAND:
                registers[CMT]=(Int)registers[GET_Ca(*this_fiber->pc)]&(Int)registers[GET_Cb(*this_fiber->pc)];
                _brh
            OR:
                registers[CMT]=(Int)registers[GET_Ca(*this_fiber->pc)]|(Int)registers[GET_Cb(*this_fiber->pc)];
                _brh
            XOR:
                registers[CMT]=(Int)registers[GET_Ca(*this_fiber->pc)]^(Int)registers[GET_Cb(*this_fiber->pc)];
                _brh
            THROW:
                this_fiber->exceptionObject = (this_fiber->sp--)->object;
                sendSignal(signal, tsig_except, 1);
                goto exception_catch;
                _brh
            CHECKNULL:
                registers[GET_Da(*this_fiber->pc)]=this_fiber->ptr == NULL || this_fiber->ptr->object==NULL;
                _brh
            RETURNOBJ:
                this_fiber->fp->object=this_fiber->ptr;
                _brh
            NEWCLASSARRAY:
                STACK_CHECK
                (++this_fiber->sp)->object = gc.newObjectArray(
                        registers[GET_Da(*this_fiber->pc)], &vm.classes[*(this_fiber->pc+1)]);
                _brh_inc(2)
            NEWSTRING:
                STACK_CHECK
                gc.createStringArray(&(++this_fiber->sp)->object,
                        vm.strings[GET_Da(*this_fiber->pc)]);
                 _brh
            ADDL:
                (this_fiber->fp+GET_Cb(*this_fiber->pc))->var+=registers[GET_Ca(*this_fiber->pc)];
                _brh
            SUBL:
                (this_fiber->fp+GET_Cb(*this_fiber->pc))->var-=registers[GET_Ca(*this_fiber->pc)];
                _brh
            MULL:
                (this_fiber->fp+GET_Cb(*this_fiber->pc))->var*=registers[GET_Ca(*this_fiber->pc)];
                _brh
            DIVL:
                (this_fiber->fp+GET_Cb(*this_fiber->pc))->var/=registers[GET_Ca(*this_fiber->pc)];
                _brh
            MODL:
                (this_fiber->fp+GET_Cb(*this_fiber->pc))->modul(registers[GET_Ca(*this_fiber->pc)]);
                _brh
            IADDL:
                (this_fiber->fp+GET_Da(*this_fiber->pc))->var+=((int32_t)*(this_fiber->pc+1));
                _brh_inc(2)
            ISUBL:
                (this_fiber->fp+GET_Da(*this_fiber->pc))->var-=((int32_t)*(this_fiber->pc+1));
                _brh_inc(2)
            IMULL:
                (this_fiber->fp+GET_Da(*this_fiber->pc))->var*=((int32_t)*(this_fiber->pc+1));
                _brh_inc(2)
            IDIVL:
                (this_fiber->fp+GET_Da(*this_fiber->pc))->var/=((int32_t)*(this_fiber->pc+1));
                _brh_inc(2)
            IMODL:
                result = (this_fiber->fp+GET_Da(*this_fiber->pc))->var;
                (this_fiber->fp+GET_Da(*this_fiber->pc))->var=result%((int32_t)*(this_fiber->pc+1));
                _brh_inc(2)
            LOADL:
                registers[GET_Ca(*this_fiber->pc)]=(this_fiber->fp+GET_Cb(*this_fiber->pc))->var;
                _brh
            IALOAD:
                CHECK_NULLVAR(
                        if(((int32_t)registers[GET_Cb(*this_fiber->pc)]) >= this_fiber->ptr->object->size || ((int32_t)registers[GET_Cb(*this_fiber->pc)]) < 0)
                            throw Exception("iaload");

                        registers[GET_Ca(*this_fiber->pc)] =
                                this_fiber->ptr->object->HEAD[(Int)registers[GET_Cb(*this_fiber->pc)]];
                )
                _brh
            POPOBJ:
                CHECK_NULL(
                        *this_fiber->ptr = (this_fiber->sp--)->object;
                )
                _brh
            SMOVR:
                (this_fiber->sp+GET_Cb(*this_fiber->pc))->var=registers[GET_Ca(*this_fiber->pc)];
                _brh
            SMOVR_2:
                (this_fiber->fp+GET_Cb(*this_fiber->pc))->var=registers[GET_Ca(*this_fiber->pc)];
                _brh
            SMOVR_3:
                (this_fiber->fp+GET_Da(*this_fiber->pc))->object=this_fiber->ptr;
                _brh
            ANDL:
                (this_fiber->fp+GET_Cb(*this_fiber->pc))->andl(registers[GET_Ca(*this_fiber->pc)]);
                _brh
            ORL:
                (this_fiber->fp+GET_Cb(*this_fiber->pc))->orl(registers[GET_Ca(*this_fiber->pc)]);
                _brh
            XORL:
            (this_fiber->fp + GET_Cb(*this_fiber->pc))->xorl(registers[GET_Ca(*this_fiber->pc)]);
                _brh
            RMOV:
                CHECK_NULLVAR(
                        if(((int32_t)registers[GET_Ca(*this_fiber->pc)]) >= this_fiber->ptr->object->size || ((int32_t)registers[GET_Ca(*this_fiber->pc)]) < 0)
                            throw Exception("movn");

                        this_fiber->ptr->object->HEAD[(Int)registers[GET_Ca(*this_fiber->pc)]]=
                                registers[GET_Cb(*this_fiber->pc)];
                )
                _brh
            NEG:
                registers[GET_Ca(*this_fiber->pc)]=-registers[GET_Cb(*this_fiber->pc)];
                _brh
            SMOV:
                registers[GET_Ca(*this_fiber->pc)]=(this_fiber->sp+GET_Cb(*this_fiber->pc))->var;
                _brh
            RETURNVAL:
                (this_fiber->fp)->var=registers[GET_Da(*this_fiber->pc)];
                _brh
            ISTORE:
                STACK_CHECK
                (++this_fiber->sp)->var = ((int32_t)*(this_fiber->pc+1));
                _brh_inc(2)
            ISTOREL:
                (this_fiber->fp+GET_Da(*this_fiber->pc))->var=(int32_t)*(this_fiber->pc+1);
                _brh_inc(2)
            PUSHNULL:
                STACK_CHECK
                gc.releaseObject(&(++this_fiber->sp)->object);
                _brh
            IPUSHL:
                STACK_CHECK
                (++this_fiber->sp)->var = (this_fiber->fp+GET_Da(*this_fiber->pc))->var;
                _brh
            PUSHL:
                STACK_CHECK
                (++this_fiber->sp)->object = (this_fiber->fp+GET_Da(*this_fiber->pc))->object;
                _brh
            ITEST:
                tmpPtr = &(this_fiber->sp--)->object;
                registers[GET_Da(*this_fiber->pc)] = tmpPtr->object == (this_fiber->sp--)->object.object;
                _brh
            INVOKE_DELEGATE:
                invokeDelegate(GET_Da(*this_fiber->pc), GET_Cb(*(this_fiber->pc+1)), this, GET_Ca(*(this_fiber->pc+1)) == 1);

                context_switch_check(false)
                _brh_NOINCREMENT
            ISADD:
                (this_fiber->sp+GET_Da(*this_fiber->pc))->var+=(int32_t)*(this_fiber->pc+1);
                _brh_inc(2)
            JE:
                LONG_CALL();
                HAS_SIGNAL
                if(registers[CMT]) {
                    this_fiber->pc=this_fiber->cache+GET_Da(*this_fiber->pc);
                    context_switch_check(false)
                    _brh_NOINCREMENT
                } else  _brh
            JNE:
                LONG_CALL();
                HAS_SIGNAL
                if(registers[CMT]==0) {
                    this_fiber->pc=this_fiber->cache+GET_Da(*this_fiber->pc);
                    context_switch_check(false)
                    _brh_NOINCREMENT
                } else  _brh
            TLS_MOVL:
                this_fiber->ptr = &(this_fiber->dataStack+GET_Da(*this_fiber->pc))->object;
                _brh
            DUP:
                tmpPtr = &this_fiber->sp->object;
                (++this_fiber->sp)->object = tmpPtr;
                _brh
            POPOBJ_2:
                this_fiber->ptr = &(this_fiber->sp--)->object;
                _brh
            SWAP:
                if((this_fiber->sp-this_fiber->dataStack) >= 2) {
                    SharpObject *swappedObj = this_fiber->sp->object.object;
                    (this_fiber->sp)->object = (this_fiber->sp-1)->object;
                    (this_fiber->sp-1)->object = swappedObj;
                    double swappedVar = this_fiber->sp->var;
                    (this_fiber->sp)->var = (this_fiber->sp-1)->var;
                    (this_fiber->sp-1)->var = swappedVar;
                } else {
                    stringstream ss;
                    ss << "Illegal stack swap while sp is ( " << (x86int_t )(this_fiber->sp - this_fiber->dataStack) << ") ";
                    throw Exception(vm.ThreadStackExcept, ss.str());
                }
                _brh
            EXP:
                registers[GET_Bc(*this_fiber->pc)]=pow(registers[GET_Ba(*this_fiber->pc)], registers[GET_Bb(*this_fiber->pc)]);
                _brh
            LDC: // tested
                registers[GET_Ca(*this_fiber->pc)]=vm.constants[GET_Cb(*this_fiber->pc)];
                _brh
            IS:
                registers[GET_Da(*this_fiber->pc)] = vm.isType(this_fiber->ptr, *(this_fiber->pc+1));
                _brh_inc(2)



        }
    } catch (Exception &e) {
        sendSignal(signal, tsig_except, 1);
    }

    exception_catch:
    if(state == THREAD_KILLED) {
        sendSignal(thread_self->signal, tsig_except, 1);
        return;
    }

    if(!vm.catchException()) {
        if(returnMethod(this))
            return;
        this_fiber->pc++;
    }

    /**
     * Exception may still be live and must make its transition to low
     * level Sharp
     */
    if(this_fiber->current->isjit) {
        return;
    }

    goto run;
}

bool Thread::try_context_switch() {
   for(Int i = 0; i < this_fiber->calls; i++) {
       Frame &frame = this_fiber->callStack[i];
       if(frame.isjit && frame.returnAddress->nativeFunc)
           return false;
   }

   contextSwitching = true;
   return true;
}

void Thread::setup() {
    signal=tsig_empty;
    suspended = false;
    exited = false;
    terminated = false;
#ifdef BUILD_JIT
    Jit::tlsSetup();
#endif

    if(id != main_threadid){
        this->this_fiber = fiber::makeFiber(name, mainMethod);
        this->this_fiber->bind(this);

        if(currentThread.object != nullptr
           && IS_CLASS(currentThread.object->info)) {
            gc.createStringArray(vm.resolveField("data", vm.resolveField("name", currentThread.object)->object), name);
        }
    } else {
        vm.state = VM_RUNNING;
        setupSigHandler();
    }

    if(currentThread.object != nullptr
       && CLASS(currentThread.object->info) == vm.ThreadClass->address) {
        vm.setFieldClass("fib", currentThread.object, vm.FiberClass);
        Object *fibField = vm.resolveField("fib", currentThread.object);
        Object *threadMainField = vm.resolveField("main", currentThread.object);
        Object *threadNameField = vm.resolveField("name", currentThread.object);

        if(fibField && threadMainField && threadNameField) {
            if(id == main_threadid) {
                this_fiber->fiberObject = fibField;
            }

            Object *nameField = vm.resolveField("name", fibField->object);
            vm.setFieldVar("main", fibField->object, 0, vm.numberValue(0, threadMainField->object));
            vm.setFieldVar("id", fibField->object, 0, this_fiber->id);

            if(nameField) {
                *nameField = threadNameField;
            }
        }
    }
}

int Thread::setPriority(Thread* thread, int priority) {
    if(thread->thread != 0) {
        if(thread->priority == priority)
            return RESULT_OK;

#ifdef WIN32_

        switch(priority) {
            case THREAD_PRIORITY_HIGH:
                SetThreadPriority(thread->thread, THREAD_PRIORITY_HIGHEST);
                break;
            case THREAD_PRIORITY_NORM:
                SetThreadPriority(thread->thread, THREAD_PRIORITY_ABOVE_NORMAL);
                break;
            case THREAD_PRIORITY_LOW:
                SetThreadPriority(thread->thread, THREAD_PRIORITY_LOWEST);
                break;
            default:
                return RESULT_ILL_PRIORITY_SET;
        }
#endif
#ifdef POSIX_
        pthread_attr_t thAttr;
        int policy = 0;
        int pthread_prio = 0;

        pthread_attr_init(&thAttr);
        pthread_attr_getschedpolicy(&thAttr, &policy);

        if(priority == THREAD_PRIORITY_HIGH) {
            pthread_prio = sched_get_priority_max(policy);
        } else if(priority == THREAD_PRIORITY_LOW) {
            pthread_prio = sched_get_priority_min(policy);
        } else {
            pthread_attr_destroy(&thAttr);
            return RESULT_ILL_PRIORITY_SET;
        }

        pthread_setschedprio(thread->thread, pthread_prio);
        pthread_attr_destroy(&thAttr);
#endif
        thread->priority = priority;
        return RESULT_OK;
    }

    return RESULT_ILL_PRIORITY_SET;
}

int Thread::setPriority(int32_t id, int priority) {

    Thread* thread = getThread(id);
    if(thread == NULL || thread->terminated || thread->state==THREAD_KILLED
       || thread->state==THREAD_CREATED)//|| id <= jit_threadid)) // TODO: update when jit is active|| id <= jit_threadid)
        return RESULT_ILL_PRIORITY_SET;

    return setPriority(thread, priority);
}

bool Thread::validStackSize(size_t sz) {
    return sz >= STACK_MIN;
}

bool Thread::validInternalStackSize(size_t sz) {
    return sz >= INTERNAL_STACK_MIN;
}

int32_t Thread::generateId() {
    Thread *thread;
    bool wrapped = false;

    while(getThread(++tid)) {
        if(tid < 0) {
            if(wrapped)
                return ILL_THREAD_ID;
            else { wrapped = true; tid = jit_threadid; }
        }
    }

    if(tid > maxThreadId)
        maxThreadId = tid;
    return tid;
}

void Thread::init(string name, Int id, Method *main, bool daemon, bool initializeStack)  {
    init();
    this->name = name;
    this->id = id;
    this->daemon=daemon;
    this->mainMethod=main;
    if(initializeStack) {
        this->this_fiber = fiber::makeFiber(this->name, main);
        this->this_fiber->bind(this);
    }

#ifdef BUILD_JIT
    this->jctx = new jit_context();
#endif
#ifdef SHARP_PROF_
    if(!daemon) {
        tprof = new Profiler();
    }
#endif
}

int Thread::destroy(Thread* thread) {
    if (thread->state == THREAD_KILLED || thread->terminated)
    {
        popThread(thread);
        thread->term();
        std::free (thread);
        return RESULT_OK;
    } else {
        return RESULT_THREAD_DESTROY_FAILED;
    }
}

void Thread::enableContextSwitch(fiber *nextFib, bool enable) {
    GUARD(mutex);
    next_fiber = nextFib;
    sendSignal(signal, tsig_context_switch, (enable ? 1 : 0));
}

void Thread::waitForContextSwitch() {
    this_fiber->setAttachedThread(NULL);
    if(this_fiber->finished) {
        if(fiber::boundFiberCount(this) > 1) {
            this_fiber->setState(this, FIB_KILLED);
        }
        else return;
    } else if(this_fiber->state == FIB_RUNNING)
        this_fiber->setState(this, FIB_SUSPENDED);
    this_fiber=NULL;

    wait:
    const long sMaxRetries = 10000000;

    long retryCount = 0;
    while (next_fiber == NULL) {
        if(state == THREAD_KILLED || hasSignal(signal, tsig_kill))
            break;

        if (retryCount++ == sMaxRetries)
        {
            if(fiber::boundFiberCount(this) == 0)
                return;

            retryCount = 0;
            __os_yield();
#ifdef WIN32_
            Sleep(1);
#endif
#ifdef POSIX_
            usleep(1*POSIX_USEC_INTERVAL);
#endif
        }
    }

    {
        if(state == THREAD_KILLED || hasSignal(signal, tsig_kill))
            return;

        if(next_fiber) {
            GUARD(mutex);
            sendSignal(signal, tsig_context_switch, 0);
            contextSwitching = false;

            if(next_fiber) {
                this_fiber = next_fiber; next_fiber = NULL;
                this_fiber->setAttachedThread(this);
                this_fiber->setState(this, FIB_RUNNING);
            } else
                goto wait;
        } else {
            goto wait;
        }
    }
}

void __os_sleep(Int INTERVAL) {
    if(INTERVAL < 0) return;
#ifdef WIN32_
    Sleep(INTERVAL);
#endif
#ifdef POSIX_
    usleep(INTERVAL);
#endif
}
