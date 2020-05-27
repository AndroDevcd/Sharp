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
HashMap<Int, Thread*> Thread::threads;
size_t threadStackSize = STACK_SIZE, internalStackSize = INTERNAL_STACK_SIZE;
recursive_mutex Thread::threadsMonitor;

/*
 * Local registers for the thread to use
 */
thread_local double registers[12];

void Thread::Startup() {
    threads.~HashMap();
    threads.init(THREAD_MAP_SIZE);

    Thread* main = (Thread*)malloc(
            sizeof(Thread)*1);
    main->Create("Main", vm.getMainMethod());
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
    if(method->paramSize>0)
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
    GUARD(threadsMonitor);
    threads.put(thread->id, thread);
}

void Thread::popThread(Thread *thread) {
    GUARD(threadsMonitor);
    threads.remove(thread->id);
}

Thread *Thread::getThread(int32_t id) {
    GUARD(threadsMonitor);
    Thread *thread;
    threads.get(id, thread);

    return thread;
}

void Thread::suspendSelf() {
    return;
//    thread_self->suspended = true;
//    sendSignal(thread_self->signal, tsig_suspend, 0);

    /*
	 * We call wait upon suspend. This function will
	 * sleep the thread most of the time. unsuspendThread() or
	 * resumeAllThreads() should be called to revive thread.
	 */
//    thread_self->wait();
}

void Thread::wait() {
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
            Sleep(5);
#endif
#ifdef POSIX_
            usleep(5*POSIX_USEC_INTERVAL);
#endif
        } else if(this->state == THREAD_KILLED) {
            this->suspended = false;
            return;
        }
    }

    this->state = THREAD_RUNNING;
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

int Thread::destroy(int32_t id) {
    if(id == thread_self->id || id==main_threadid)
        return RESULT_ILL_THREAD_DESTROY; // cannot destroy self or main

    Thread* thread = getThread(id);
    if(thread == NULL || thread->daemon)
        return RESULT_ILL_THREAD_DESTROY;

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

void Thread::waitForThreadSuspend(Thread *thread) {
    const int sMaxRetries = 10000000;
    const int sMaxSpinCount = 25;

    int spinCount = 0;
    int retryCount = 0;

    suspendThread(thread);
    while (thread->state == THREAD_RUNNING && !thread->suspended)
    {
        if (retryCount++ == sMaxRetries)
        {
            retryCount = 0;
            if(++spinCount >= sMaxSpinCount || thread->state >= THREAD_SUSPENDED)
                return;
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

            __os_sleep(1);
        }
    }
}

void Thread::terminateAndWaitForThreadExit(Thread *thread) {
    const int sMaxRetries = 10000000;
    int retryCount = 0;

    thread->term();

    GUARD(thread->mutex);
    thread->state = THREAD_KILLED;
    sendSignal(thread->signal, tsig_kill, 1);

    while (!thread->exited)
    {
        if (retryCount++ == sMaxRetries)
        {
            retryCount = 0;
            if(thread->exited)
                return;

            __os_sleep(1);
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
            __os_sleep(1);
        }
    }

    return RESULT_OK;
}

void Thread::suspendAllThreads() {
    GUARD(threadsMonitor);
    Thread* thread;

    for(uInt i = 0; i < maxThreadId; i++) {

        if(threads.get(i, thread)
            && thread != NULL
            && (thread->id != thread_self->id)
            && thread->state == THREAD_RUNNING){
            suspendThread(thread);
            waitForThreadSuspend(thread);
        }
    }
}

void Thread::resumeAllThreads() {
    GUARD(threadsMonitor);
    Thread* thread;

    for(unsigned int i= 0; i < maxThreadId; i++) {

        if(threads.get(i, thread)
           && thread != NULL
           && (thread->id != thread_self->id)){
            unsuspendThread(thread);
        }
    }
}

int Thread::unsuspendThread(Thread *thread) {
    thread->suspended = false;
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
    this->state = THREAD_KILLED;
    sendSignal(this->signal, tsig_kill, 1);
    this->terminated = true;
    if(dataStack != NULL) {
        std::free(dataStack); dataStack = NULL;
    }

    if(callStack != NULL) {
        std::free(callStack); callStack = NULL;
    }

    if(rand != NULL) {
        delete rand;
    }

#ifdef SHARP_PROF_
    tprof->free();
    delete tprof;
#endif
    this->name.free();

#ifdef BUILD_JIT
    delete jctx;
#endif
}

int Thread::join(int32_t id) {
    if (thread_self != NULL && (id == thread_self->id || id <= jit_threadid))
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

    if (thread->state == THREAD_RUNNING)
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
    Thread* thread;
    suspendAllThreads();

    for(unsigned int i = 0; i < maxThreadId; i++) {
        threads.get(i, thread);

        if(threads.get(i, thread) && thread != NULL) {
            if(thread->id != thread_self->id
               && thread->state != THREAD_KILLED && thread->state != THREAD_CREATED) {
                if(thread->state == THREAD_RUNNING){
                    interrupt(thread); // shouldn't happen
                }

                terminateAndWaitForThreadExit(thread);
            } else {
                thread->term();
            }
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
            thread->state = THREAD_KILLED; // terminate thread
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

        for(Int i = 0; i < maxThreadId; i++) {
            threads.get(i, thread);
            std::free(thread);
        }
    }
}

void Thread::exit() {
    GUARD(threadsMonitor);
    if(hasSignal(signal, tsig_except)) {
        this->exitVal = 1;

        Object* stackTrace = vm.resolveField("stack_trace", exceptionObject.object);
        Object* message = vm.resolveField("message", exceptionObject.object);
        ClassObject *k = &vm.classes[CLASS(exceptionObject.object->info)];
        cout << "Unhandled exception on thread " << name.str() << " (most recent call last):\n"; cout
                << (stackTrace != NULL ? vm.stringValue(stackTrace->object) : "");
        cout << endl << (&vm.classes[CLASS(exceptionObject.object->info)])->name.str() << " ("
           << (message != NULL ? vm.stringValue(message->object) : "") << ")\n";
    } else {
        if(!daemon && dataStack != NULL)
            this->exitVal = (int)dataStack[vm.manifest.threadLocals].var;
        else
            this->exitVal = 0;
    }

    if(dataStack != NULL) {
        GarbageCollector::self->freeMemory(sizeof(StackElement) * stackLimit);
        StackElement *p = dataStack;
        for(size_t i = 0; i < stackLimit; i++)
        {
            if(p->object.object) {
                DEC_REF(p->object.object);
                p->object.object=NULL;
            }
            p++;
        }
    }

    if(callStack) {
        free(this->callStack); callStack = NULL;
        GarbageCollector::self->freeMemory(sizeof(Frame) * stackLimit);
    }
    this->state = THREAD_KILLED;
    this->signal = tsig_empty;

    releaseResources();
    this->exited = true;
}

void Thread::releaseResources() {
    GarbageCollector::self->reconcileLocks(this);
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
    cout << "sp -> " << (thread_self->sp-thread_self->dataStack) << endl;
    cout << "fp -> " << (thread_self->fp-thread_self->dataStack) << endl;
    cout << "pc -> " << PC(thread_self) << endl;
    cout << "current function -> " << thread_self->current->fullName.str() << endl;

    native_string stackTrace;
    vm.fillStackTrace(stackTrace);
    cout << "call stack (most recent call last):\n" << stackTrace.str() << endl;
//    if(thread_self->current != NULL) {
//        cout << "current -> " << thread_self->current->name.str() << endl;
//    }
//    native_string stackTrace;
//
//    vm.fillStackTrace(stackTrace);
//    cout << "stacktrace ->\n\n " << stackTrace.str() << endl;
//    cout << endl;
//
//    for(long i = 0; i < 15; i++) {
//        cout << "fp.var [" << i << "] = " << thread_self->dataStack[i].var << ";" << endl;
//    }
}

void printStack() {
    native_string str;
    vm.fillStackTrace(str);
    cout << " stack\n" << str.str() << endl << std::flush;
}
#endif


string getVarCastExceptionMsg(DataType varType, bool isArray) {
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
    ss << "illegal cast to "<< type << (isArray ? "[]" : "");
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

    Object *tmpPtr;
    SharpObject* tmpShObj;
    Object *ptr;
    Int result;
    fptr jitFun;

#ifdef SHARP_PROF_
    tprof->init(stack_lmt);
    tprof->starttm=Clock::realTimeInNSecs();
    for(size_t i = 0; i < manifest.methods; i++) {
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
//        DISPATCH();

        for (;;) {
            top:
                if(current->address == 228 && PC(this) >= 86) {
                    Int i = 0;
                }
                DISPATCH();
            _NOP: // tested
                _brh
            _INT:

#ifdef SHARP_PROF_
            if(GET_Da(*pc) == 0xa9) {
                tprof->endtm=Clock::realTimeInNSecs();
                tprof->profile();
                tprof->dump();
            }

#endif
                VirtualMachine::sysInterrupt(GET_Da(*pc));
                if(vm.state == VM_TERMINATED) return;
                _brh
            _MOVI: // tested
                registers[GET_Da(*pc)]=(int32_t)*(pc+1);
                _brh_inc(2)
            RET: // tested
                result = GET_Da(*pc); // error state
                if(result == ERR_STATE && exceptionObject.object == NULL) {
                    exceptionObject
                            = (sp--)->object.object;
                }

                if(returnMethod(this)) {
                    // handle the exception even if we hit the last method or return back to JIT
                    if(result == ERR_STATE) sendSignal(signal, tsig_except, 1);
                    return;
                }

                if(result == ERR_STATE) {
                    sendSignal(signal, tsig_except, 1);
                    goto exception_catch;
                }
                LONG_CALL();
                _brh
            HLT: // tested
                sendSignal(signal, tsig_kill, 1);
                _brh
            NEWARRAY: // tested
                STACK_CHECK
                (++sp)->object =
                        GarbageCollector::self->newObject(registers[GET_Da(*pc)]);
                _brh
            CAST:
                CHECK_NULL(ptr->castObject(registers[GET_Da(*pc)]);)
                _brh
            VARCAST:
                CHECK_NULL2(
                        if(TYPE(ptr->object->info) != _stype_var) {
                            throw Exception(vm.ClassCastExcept,
                                    getVarCastExceptionMsg((DataType)GET_Ca(*pc), GET_Cb(*pc)));
                        }
                )
                _brh
            MOV8: // tested
                registers[GET_Ca(*pc)]=(int8_t)registers[GET_Cb(*pc)];
                _brh
            MOV16: // tested
                registers[GET_Ca(*pc)]=(int16_t)registers[GET_Cb(*pc)];
                _brh
            MOV32: // tested
                registers[GET_Ca(*pc)]=(int32_t)registers[GET_Cb(*pc)];
                _brh
            MOV64: // tested
                registers[GET_Ca(*pc)]=(Int)registers[GET_Cb(*pc)];
                _brh
            MOVU8: // tested
                registers[GET_Ca(*pc)]=(uint8_t)registers[GET_Cb(*pc)];
                _brh
            MOVU16: // tested
                registers[GET_Ca(*pc)]=(uint16_t)registers[GET_Cb(*pc)];
                _brh
            MOVU32: // tested
                registers[GET_Ca(*pc)]=(uint32_t)registers[GET_Cb(*pc)];
                _brh
            MOVU64: // tested
                registers[GET_Ca(*pc)]=(uInt)registers[GET_Cb(*pc)];
                _brh
            RSTORE: // tested
                STACK_CHECK
                (++sp)->var = registers[GET_Da(*pc)];
                 _brh
            ADD: // tested
                registers[GET_Bc(*pc)]=registers[GET_Ba(*pc)]+registers[GET_Bb(*pc)];
                _brh
            SUB: // tested
                registers[GET_Bc(*pc)]=registers[GET_Ba(*pc)]-registers[GET_Bb(*pc)];
                _brh
            MUL: // tested
                registers[GET_Bc(*pc)]=registers[GET_Ba(*pc)]*registers[GET_Bb(*pc)];
                _brh
            DIV: // tested
                if(registers[GET_Ba(*pc)]==0 && registers[GET_Bb(*pc)]==0) throw Exception("divide by 0");
                registers[GET_Bc(*pc)]=registers[GET_Ba(*pc)]/registers[GET_Bb(*pc)];
                _brh
            MOD: // tested
                registers[GET_Bc(*pc)]=(Int)registers[GET_Ba(*pc)]%(Int)registers[GET_Bb(*pc)];
                _brh
            IADD: // tested
                registers[GET_Da(*pc)]+=(int32_t)*(pc+1);
                _brh_inc(2)
            ISUB: // tested
                registers[GET_Da(*pc)]-=(int32_t)*(pc+1);
                _brh_inc(2)
            IMUL: // tested
                registers[GET_Da(*pc)]*=(int32_t)*(pc+1);
                _brh_inc(2)
            IDIV: // tested
                if(((int32_t)*(pc+1))==0) throw Exception("divide by 0");
                registers[GET_Da(*pc)]/=(int32_t)*(pc+1);
                _brh_inc(2)
            IMOD: // tested
                registers[GET_Da(*pc)]=(Int)registers[GET_Da(*pc)]%(Int)*(pc+1);
                _brh_inc(2)
            POP: // tested
                --sp;
                _brh
            INC: // tested
                registers[GET_Da(*pc)]++;
                _brh
            DEC: // tested
                registers[GET_Da(*pc)]--;
                _brh
            MOVR: // tested
                registers[GET_Ca(*pc)]=registers[GET_Cb(*pc)];
                _brh
            BRH: // tested
                pc=cache+(Int)registers[ADX];
                LONG_CALL();
                _brh_NOINCREMENT
            IFE:
                LONG_CALL();
                if(registers[CMT]) {
                    pc=cache+(int64_t)registers[ADX]; _brh_NOINCREMENT
                } else  _brh
            IFNE:
                LONG_CALL();
                if(registers[CMT]==0) {
                    pc=cache+(int64_t)registers[ADX]; _brh_NOINCREMENT
                } else  _brh
            LT:
                registers[CMT]=registers[GET_Ca(*pc)]<registers[GET_Cb(*pc)];
                _brh
            GT:
                registers[CMT]=registers[GET_Ca(*pc)]>registers[GET_Cb(*pc)];
                _brh
            LTE:
                registers[CMT]=registers[GET_Ca(*pc)]<=registers[GET_Cb(*pc)];
                _brh
            GTE:
                registers[CMT]=registers[GET_Ca(*pc)]>=registers[GET_Cb(*pc)];
                _brh
            MOVL: // tested
                ptr = &(fp+GET_Da(*pc))->object;
                _brh
            POPL: // tested
                (fp+GET_Da(*pc))->object
                        = (sp--)->object.object;
                _brh
            IPOPL: // tested
                (fp+GET_Da(*pc))->var
                        = (sp--)->var;
                _brh
            MOVSL: // tested
                ptr = &((sp+GET_Da(*pc))->object);
                _brh
            SIZEOF:
                if(ptr==NULL || ptr->object == NULL)
                    registers[GET_Da(*pc)] = 0;
                else
                    registers[GET_Da(*pc)]=ptr->object->size;
                _brh
            PUT: // tested
                cout << registers[GET_Da(*pc)];
                _brh
            PUTC:
                printf("%c", (char)registers[GET_Da(*pc)]);
                _brh
            GET:
                if(_64CMT)
                    registers[GET_Da(*pc)] = getche();
                else
                    registers[GET_Da(*pc)] = getch();
                _brh
            CHECKLEN:
                CHECK_NULL2(
                        if((registers[GET_Da(*pc)]<ptr->object->size) &&!(registers[GET_Da(*pc)]<0)) { _brh }
                        else {
                            stringstream ss;
                            ss << "Access to Object at: " << registers[GET_Da(*pc)] << " size is " << ptr->object->size;
                            throw Exception(vm.IndexOutOfBoundsExcept, ss.str());
                        }
                )
            JMP: // tested
                pc = cache+GET_Da(*pc);
                LONG_CALL();
                _brh_NOINCREMENT
            LOADPC: // tested
                registers[GET_Da(*pc)] = PC(this);
                _brh
            PUSHOBJ:
                STACK_CHECK
                (++sp)->object = ptr;
                _brh
            DEL:
                GarbageCollector::self->releaseObject(ptr);
                _brh
            CALL:
#ifdef SHARP_PROF_
            tprof->hit(vm.methods+GET_Da(*pc));
#endif
                if((jitFun = executeMethod(GET_Da(*pc), this)) != NULL) {

#ifdef BUILD_JIT
                    jitFun(jctx);
#endif
                }
                _brh_NOINCREMENT
            CALLD:
#ifdef SHARP_PROF_
            tprof->hit(vm.methods+GET_Da(*pc));
#endif
                if((result = (int64_t )registers[GET_Da(*pc)]) <= 0 || result >= vm.manifest.methods) {
                    stringstream ss;
                    ss << "invalid call to pointer of " << result;
                    throw Exception(ss.str());
                }
                if((jitFun = executeMethod(result, this)) != NULL) {

#ifdef BUILD_JIT
                    jitFun(jctx);
#endif
                }
                _brh_NOINCREMENT
            NEWCLASS:
                STACK_CHECK
                (++sp)->object =
                        GarbageCollector::self->newObject(&vm.classes[GET_Da(*pc)]);
                _brh
            MOVN:
                CHECK_NULLOBJ(
                        if(((int32_t)*(pc+1)) >= ptr->object->size || ((int32_t)*(pc+1)) < 0)
                            throw Exception("movn");

                        ptr = &ptr->object->node[((int32_t)*(pc+1))];
                )
                _brh_inc(2)
            SLEEP:
                __os_sleep((Int)registers[GET_Da(*pc)]);
                _brh
            TEST:
                registers[CMT]=registers[GET_Ca(*pc)]==registers[GET_Cb(*pc)];
                _brh
            TNE:
                registers[CMT]=registers[GET_Ca(*pc)]!=registers[GET_Cb(*pc)];
                _brh
            LOCK:
                CHECK_NULL2(Object::monitorLock(ptr, this);)
                _brh
            ULOCK:
                CHECK_NULL2(Object::monitorUnLock(ptr, this);)
                _brh
            MOVG:
                ptr = vm.staticHeap+GET_Da(*pc);
                _brh
            MOVND:
                CHECK_NULLOBJ(ptr = &ptr->object->node[(Int)registers[GET_Da(*pc)]];)
                _brh
            NEWOBJARRAY:
                (++sp)->object = GarbageCollector::self->newObjectArray(registers[GET_Da(*pc)]);
                STACK_CHECK _brh
            NOT:
                registers[GET_Ca(*pc)]=!registers[GET_Cb(*pc)];
                _brh
            SKIP:
                pc = pc+GET_Da(*pc);
                _brh
            LOADVAL:
                registers[GET_Da(*pc)]=(sp--)->var;
                _brh
            SHL:
                registers[GET_Bc(*pc)]=(int64_t)registers[GET_Ba(*pc)]<<(int64_t)registers[GET_Bb(*pc)];
                _brh
            SHR:
                registers[GET_Bc(*pc)]=(int64_t)registers[GET_Ba(*pc)]>>(int64_t)registers[GET_Bb(*pc)];
                _brh
            SKPE:
                LONG_CALL();
                if(registers[GET_Ca(*pc)]) {
                    pc = pc+GET_Cb(*pc); _brh_NOINCREMENT
                } else _brh
            SKNE:
                LONG_CALL();
                if(registers[GET_Ca(*pc)]==0) {
                    pc = pc+GET_Cb(*pc); _brh_NOINCREMENT
                } else _brh
            CMP:
                registers[CMT]=registers[GET_Da(*pc)]==((int32_t)*(pc+1));
                _brh_inc(2)
            AND:
                registers[CMT]=registers[GET_Ca(*pc)]&&registers[GET_Cb(*pc)];
                _brh
            UAND:
                registers[CMT]=(Int)registers[GET_Ca(*pc)]&(Int)registers[GET_Cb(*pc)];
                _brh
            OR:
                registers[CMT]=(Int)registers[GET_Ca(*pc)]|(Int)registers[GET_Cb(*pc)];
                _brh
            XOR:
                registers[CMT]=(Int)registers[GET_Ca(*pc)]^(Int)registers[GET_Cb(*pc)];
                _brh
            THROW:
                exceptionObject = (sp--)->object;
                prepareException();
                goto exception_catch;
                _brh
            CHECKNULL:
                registers[GET_Da(*pc)]=ptr == NULL || ptr->object==NULL;
                _brh
            RETURNOBJ:
                fp->object=ptr;
                _brh
            NEWCLASSARRAY:
                STACK_CHECK
                (++sp)->object = GarbageCollector::self->newObjectArray(
                        registers[GET_Ca(*pc)], &vm.classes[GET_Cb(*pc)]);
                _brh
            NEWSTRING:
                STACK_CHECK
                GarbageCollector::self->createStringArray(&(++sp)->object,
                        vm.strings[GET_Da(*pc)]);
                 _brh
            ADDL:
                (fp+GET_Cb(*pc))->var+=registers[GET_Ca(*pc)];
                _brh
            SUBL:
                (fp+GET_Cb(*pc))->var-=registers[GET_Ca(*pc)];
                _brh
            MULL:
                (fp+GET_Cb(*pc))->var*=registers[GET_Ca(*pc)];
                _brh
            DIVL:
                (fp+GET_Cb(*pc))->var/=registers[GET_Ca(*pc)];
                _brh
            MODL:
                (fp+GET_Cb(*pc))->modul(registers[GET_Ca(*pc)]);
                _brh
            IADDL:
                (fp+GET_Da(*pc))->var+=((int32_t)*(pc+1));
                _brh_inc(2)
            ISUBL:
                (fp+GET_Da(*pc))->var-=((int32_t)*(pc+1));
                _brh_inc(2)
            IMULL:
                (fp+GET_Da(*pc))->var*=((int32_t)*(pc+1));
                _brh_inc(2)
            IDIVL:
                (fp+GET_Da(*pc))->var/=((int32_t)*(pc+1));
                _brh_inc(2)
            IMODL:
                result = (fp+GET_Da(*pc))->var;
                (fp+GET_Da(*pc))->var=result%((int32_t)*(pc+1));
                _brh_inc(2)
            LOADL:
                registers[GET_Ca(*pc)]=(fp+GET_Cb(*pc))->var;
                _brh
            IALOAD:
                CHECK_NULLVAR(
                        registers[GET_Ca(*pc)] =
                                ptr->object->HEAD[(Int)registers[GET_Cb(*pc)]];
                )
                _brh
            POPOBJ:
                CHECK_NULL(
                        *ptr = (sp--)->object;
                )
                _brh
            SMOVR:
                (sp+GET_Cb(*pc))->var=registers[GET_Ca(*pc)];
                _brh
            SMOVR_2:
                (fp+GET_Cb(*pc))->var=registers[GET_Ca(*pc)];
                _brh
            SMOVR_3:
                (fp+GET_Da(*pc))->object=ptr;
                _brh
            ANDL:
                (fp+GET_Cb(*pc))->andl(registers[GET_Ca(*pc)]);
                _brh
            ORL:
                (fp+GET_Cb(*pc))->orl(registers[GET_Ca(*pc)]);
                _brh
            XORL:
            (fp + GET_Cb(*pc))->xorl(registers[GET_Ca(*pc)]);
                _brh
            RMOV:
                CHECK_NULLVAR(
                        ptr->object->HEAD[(Int)registers[GET_Ca(*pc)]]=
                                registers[GET_Cb(*pc)];
                )
                _brh
            NEG:
                registers[GET_Ca(*pc)]=-registers[GET_Cb(*pc)];
                _brh
            SMOV:
                registers[GET_Ca(*pc)]=(sp+GET_Cb(*pc))->var;
                _brh
            RETURNVAL:
                (fp)->var=registers[GET_Da(*pc)];
                _brh
            ISTORE:
                STACK_CHECK
                (++sp)->var = ((int32_t)*(pc+1));
                _brh_inc(2)
            ISTOREL:
                (fp+GET_Da(*pc))->var=(int32_t)*(pc+1);
                _brh_inc(2)
            PUSHNULL:
                STACK_CHECK
                GarbageCollector::self->releaseObject(&(++sp)->object);
                _brh
            IPUSHL:
                STACK_CHECK
                (++sp)->var = (fp+GET_Da(*pc))->var;
                _brh
            PUSHL:
                STACK_CHECK
                (++sp)->object = (fp+GET_Da(*pc))->object;
                _brh
            ITEST:
                tmpPtr = &(sp--)->object;
                registers[GET_Da(*pc)] = tmpPtr->object == (sp--)->object.object;
                _brh
            INVOKE_DELEGATE:
                invokeDelegate(GET_Da(*pc), GET_Cb(*(pc+1)), this, GET_Ca(*(pc+1)));
                _brh_NOINCREMENT
            ISADD:
                (sp+GET_Da(*pc))->var+=(int32_t)*(pc+1);
                _brh_inc(2)
            JE:
                LONG_CALL();
                if(registers[CMT]) {
                    pc=cache+GET_Da(*pc); _brh_NOINCREMENT
                } else  _brh
            JNE:
                LONG_CALL();
                if(registers[CMT]==0) {
                    pc=cache+GET_Da(*pc); _brh_NOINCREMENT
                } else  _brh
            TLS_MOVL:
                ptr = &(dataStack+GET_Da(*pc))->object;
                _brh
            DUP:
                tmpPtr = &sp->object;
                (++sp)->object = tmpPtr;
                _brh
            POPOBJ_2:
                ptr = &(sp--)->object;
                _brh
            SWAP:
                if((sp-dataStack) >= 2) {
                    SharpObject *swappedObj = sp->object.object;
                    (sp)->object = (sp-1)->object;
                    (sp-1)->object = swappedObj;
                } else {
                    stringstream ss;
                    ss << "Illegal stack swap while sp is ( " << (x86int_t )(sp - dataStack) << ") ";
                    throw Exception(vm.ThreadStackExcept, ss.str());
                }
                _brh
            EXP:
                registers[GET_Bc(*pc)]=pow(registers[GET_Ba(*pc)], registers[GET_Bb(*pc)]);
                _brh
            LDC: // tested
                registers[GET_Ca(*pc)]=vm.constants[GET_Cb(*pc)];
                _brh



        }
    } catch (Exception &e) {
        prepareException();
    }

    exception_catch:
    if(state == THREAD_KILLED) {
        sendSignal(thread_self->signal, tsig_except, 1);
        return;
    }

    if(!vm.catchException()) {
        if(returnMethod(this))
            return;
        pc++;
    }

    /**
     * Exception may still be live and must make its transition to low
     * level Sharp
     */
    if(current->isjit) {
        return;
    }

    goto run;
}

void Thread::setup() {
    current = NULL;
    calls=-1;
    signal=tsig_empty;
    suspended = false;
    exited = false;
    terminated = false;
    exitVal = 0;
#ifdef BUILD_JIT
    Jit::tlsSetup();
#endif

    if(dataStack==NULL) {
        dataStack = (StackElement*)__malloc(sizeof(StackElement) * stackLimit);
        GarbageCollector::self->addMemory(sizeof(StackElement) * stackLimit);
    }
    if(callStack==NULL) {
        callStack = (Frame*)__malloc(sizeof(Frame) * stackLimit);
        GarbageCollector::self->addMemory(sizeof(Frame) * stackLimit);
    }
    if(id != main_threadid){
        int priority = (int)vm.resolveField("priority", currentThread.object)->object->HEAD[0];
        setPriority(this, priority);

        if(currentThread.object != nullptr
           && IS_CLASS(currentThread.object->info)) {
            GarbageCollector::self->createStringArray(vm.resolveField("name", currentThread.object), name);
        }
        fp=&dataStack[vm.manifest.threadLocals];
        sp=(&dataStack[vm.manifest.threadLocals])-1;

        for(unsigned long i = 0; i < stackLimit; i++) {
            this->dataStack[i].object.object = NULL;
            this->dataStack[i].var=0;
        }
    } else {
        vm.state = VM_RUNNING;
        GarbageCollector::self->addMemory(sizeof(StackElement) * stackLimit);
        setupSigHandler();
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
                SetThreadPriority(thread->thread, THREAD_PRIORITY_BELOW_NORMAL);
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
        return RESULT_OK;
    }

    return RESULT_ILL_PRIORITY_SET;
}

void Thread::prepareException() {
    vm.fillStackTrace(&exceptionObject);
    sendSignal(signal, tsig_except, 1);
}

int Thread::setPriority(int32_t id, int priority) {

    Thread* thread = getThread(id);
    if(thread == NULL || thread->terminated || thread->state==THREAD_KILLED
       || thread->state==THREAD_CREATED || id <= jit_threadid)
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
    GUARD(threadsMonitor);

    Thread *thread;
    bool wrapped = false;

    while(threads.get(++tid, thread)) {
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
    this->main = main;
    this->stackLimit = internalStackSize;
    this->daemon=daemon;

#ifdef BUILD_JIT
    this->jctx = new jit_context();
#endif
#ifdef SHARP_PROF_
    if(!daemon) {
        thread->tprof = new Profiler();
    }
#endif

    if(initializeStack) {
        this->dataStack = (StackElement *) __calloc(internalStackSize, sizeof(StackElement));
        this->fp = &dataStack[vm.manifest.threadLocals];
        this->sp = (&dataStack[vm.manifest.threadLocals]) - 1;
    }
}

void __os_sleep(Int INTERVAL) {
#ifdef WIN32_
    Sleep(INTERVAL);
#endif
#ifdef POSIX_
    usleep(INTERVAL);
#endif
}
