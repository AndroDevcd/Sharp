//
// Created by BraxtonN on 2/12/2018.
//

#include <cmath>
#include "Thread.h"
#include "memory/GarbageCollector.h"
#include "Exe.h"
#include "Environment.h"
#include "VirtualMachine.h"
#include "Opcode.h"
#include "register.h"
#include "Manifest.h"
#include "oo/Object.h"
#include "../util/time.h"
#include "jit/_BaseAssembler.h"
#include "jit/Jit.h"

#ifdef WIN32_
#include <conio.h>
#elif defined(POSIX_)
#include "termios.h"
#endif

Int Thread::tid = ILL_THREAD_ID;
uInt Thread::maxThreadId = 0;
thread_local Thread* thread_self = NULL;
HashMap<Int, Thread*> Thread::threads;
size_t threadStackSize = STACK_SIZE, internalStackSize = INTERNAL_STACK_SIZE;

#ifdef WIN32_
std::mutex Thread::threadsMonitor;
#endif
#ifdef POSIX_
std::mutex Thread::threadsMonitor;
#endif

/*
 * Local registers for the thread to use
 */
thread_local double registers[12];

void Thread::Startup() {
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
    std::lock_guard<std::mutex> guard(threadsMonitor);
    threads.put(thread->id, thread);
}

void Thread::popThread(Thread *thread) {
    std::lock_guard<std::mutex> guard(threadsMonitor);
    threads.remove(thread->id);
}

Thread *Thread::getThread(int32_t id) {
    std::lock_guard<std::mutex> guard(threadsMonitor);
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
    pthread_attr_setstacksize(&attr,thread->stack + STACK_OVERFLOW_BUF);
    if(pthread_create( &thread->thread, &attr, vm->InterpreterThreadStart, (void*) thread))
        return RESULT_THREAD_NOT_STARTED; // thread was not started
    else {
        return waitForThread(thread);
    }
#endif

}

int Thread::destroy(int64_t id) {
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

    std::lock_guard<std::mutex> gd(thread->mutex);
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

    while (thread->state == THREAD_CREATED
           || thread->state == THREAD_SUSPENDED)
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
    std::lock_guard<std::mutex> guard(threadsMonitor);
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
    std::lock_guard<std::mutex> guard(threadsMonitor);
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
        std::lock_guard<std::mutex> gd(thread->mutex);
        sendSignal(thread->signal, tsig_suspend, 1);
    }
}

void Thread::term() {
    std::lock_guard<std::mutex> gd(mutex);
    this->state = THREAD_KILLED;
    sendSignal(this->signal, tsig_kill, 1);
    this->terminated = true;
    if(dataStack != NULL) {
        for(unsigned long i = 0; i < this->stackLimit; i++) {
            GarbageCollector::self->releaseObject(&dataStack[i].object);
        }
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
    std::lock_guard<std::mutex> guard(threadsMonitor);
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
            std::lock_guard<std::mutex> gd(thread->mutex);
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
    std::lock_guard<std::mutex> guard(threadsMonitor);
    if(hasSignal(signal, tsig_except)) {
        this->exitVal = 1;

        cout << "Unhandled exception on thread " << name.str() << " (most recent call last):\n"; cout
                << throwable.stackTrace.str();
        cout << endl << throwable.handlingClass->name.str() << " ("
           << throwable.message.str() << ")\n";
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
    cout << "adx = " << registers[i64adx] << endl;
    cout << "cx = " << registers[i64cx] << endl;
    cout << "cmt = " << registers[i64cmt] << endl;
    cout << "ebx = " << registers[i64ebx] << endl;
    cout << "ecx = " << registers[i64ecx] << endl;
    cout << "ecf = " << registers[i64ecf] << endl;
    cout << "edf = " << registers[i64edf] << endl;
    cout << "ehf = " << registers[i64ehf] << endl;
    cout << "bmr = " << registers[i64bmr] << endl;
    cout << "egx = " << registers[i64egx] << endl;
    cout << "sp -> " << (thread_self->sp-thread_self->dataStack) << endl;
    cout << "fp -> " << (thread_self->fp-thread_self->dataStack) << endl;
    cout << "pc -> " << PC(thread_self) << endl;
    if(thread_self->current != NULL) {
        cout << "current -> " << thread_self->current->name.str() << endl;
    }
    native_string stackTrace;

    vm->fillStackTrace(stackTrace);
    cout << "stacktrace ->\n\n " << stackTrace.str() << endl;
    cout << endl;

    for(long i = 0; i < 15; i++) {
        cout << "fp.var [" << i << "] = " << thread_self->dataStack[i].var << ";" << endl;
    }
}

void printStack() {
    native_string str;
    vm->fillStackTrace(str);
    cout << " stack\n" << str.str() << endl << std::flush;
}
#endif


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
/**
 * This tells the virtual machine to process code
 * when executing a finally block or the regular processing
 * of a function.
 */
thread_local short startAddress = 0;
/*
 * We need this to keep track of which finally block we are executing
 */
FinallyTable finallyTable;
unsigned long long irCount = 0, overflow = 0;

void Thread::exec() {

    register int64_t tmp=0;
    register int64_t val=0;
    register int64_t delegate=0;
    register int64_t args=0;
    ClassObject *klass;
    SharpObject* o=NULL;
    Method* f;
    fptr jitFn;
    Object* o2=NULL;
    void* opcodeStart = (startAddress == 0) ?  (&&interp) : (&&finally) ;
    Method* finnallyMethod;

#ifdef SHARP_PROF_
    tprof->init(stack_lmt);
    tprof->starttm=Clock::realTimeInNSecs();
    for(size_t i = 0; i < manifest.methods; i++) {
        funcProf prof = funcProf(env->methods+i);
        tprof->functions.push_back(prof);
    }
#endif

#ifdef SHARP_PROF_
    tprof->hit(main);
#endif

    _initOpcodeTable
    try {
        for (;;) {
            /* We dont want to always run finally code when we start a thread */
            if(startAddress == 0) goto interp;
            finnallyMethod = current;

            finally:
            if((PC(this) <finallyTable.start_pc || PC(this) > finallyTable.end_pc)
               && current==finnallyMethod)
                return;

            interp:
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
                if(masterShutdown) return;
                _brh
            _MOVI: // tested
                registers[*(pc+1)]=GET_Da(*pc);
                _brh_inc(2)
            RET: // tested
                if(returnMethod(this))
                    return;
                LONG_CALL();
                _brh
            HLT: // tested
                state=THREAD_KILLED;
                sendSignal(signal, tsig_kill, 1);
                _brh
            NEWARRAY: // tested
                (++sp)->object =
                        GarbageCollector::self->newObject(registers[GET_Da(*pc)]);
                STACK_CHECK _brh
            CAST:
                CHECK_NULL(o2->castObject(registers[GET_Da(*pc)]);)
                _brh
            VARCAST:
                CHECK_NULL2(
                        if(TYPE(o2->object->info) != _stype_var) {
                            stringstream ss;
                            ss << "illegal cast to var" << (GET_Da(*pc) ? "[]" : "");
                            throw Exception(Environment::ClassCastException, ss.str());
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
                registers[GET_Ca(*pc)]=(int64_t)registers[GET_Cb(*pc)];
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
                registers[GET_Ca(*pc)]=(uint64_t)registers[GET_Cb(*pc)];
                _brh
            RSTORE: // tested
                (++sp)->var = registers[GET_Da(*pc)];
                STACK_CHECK _brh
            ADD: // tested
                registers[*(pc+1)]=registers[GET_Ca(*pc)]+registers[GET_Cb(*pc)];
                _brh_inc(2)
            SUB: // tested
                registers[*(pc+1)]=registers[GET_Ca(*pc)]-registers[GET_Cb(*pc)];
                _brh_inc(2)
            MUL: // tested
                registers[*(pc+1)]=registers[GET_Ca(*pc)]*registers[GET_Cb(*pc)];
                _brh_inc(2)
            DIV: // tested
                if(registers[GET_Ca(*pc)]==0 && registers[GET_Cb(*pc)]==0) throw Exception("divide by 0");
                registers[*(pc+1)]=registers[GET_Ca(*pc)]/registers[GET_Cb(*pc)];
                _brh_inc(2)
            MOD: // tested
                registers[*(pc+1)]=(int64_t)registers[GET_Ca(*pc)]%(int64_t)registers[GET_Cb(*pc)];
                _brh_inc(2)
            IADD: // tested
                registers[GET_Ca(*pc)]+=GET_Cb(*pc);
                _brh
            ISUB: // tested
                registers[GET_Ca(*pc)]-=GET_Cb(*pc);
                _brh
            IMUL: // tested
                registers[GET_Ca(*pc)]*=GET_Cb(*pc);
                _brh
            IDIV: // tested
                if(GET_Cb(*pc)==0) throw Exception("divide by 0");
                registers[GET_Ca(*pc)]/=GET_Cb(*pc);
                _brh
            IMOD: // tested
                registers[GET_Ca(*pc)]=(int64_t)registers[GET_Ca(*pc)]%(int64_t)GET_Cb(*pc);
                _brh
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
                pc=cache+(int64_t)registers[i64adx];
                LONG_CALL();
                _brh_NOINCREMENT
            IFE:
                LONG_CALL();
                if(registers[i64cmt]) {
                    pc=cache+(int64_t)registers[i64adx]; _brh_NOINCREMENT
                } else  _brh
            IFNE:
                LONG_CALL();
                if(registers[i64cmt]==0) {
                    pc=cache+(int64_t)registers[i64adx]; _brh_NOINCREMENT
                } else  _brh
            LT:
                registers[i64cmt]=registers[GET_Ca(*pc)]<registers[GET_Cb(*pc)];
                _brh
            GT:
                registers[i64cmt]=registers[GET_Ca(*pc)]>registers[GET_Cb(*pc)];
                _brh
            LTE:
                registers[i64cmt]=registers[GET_Ca(*pc)]<=registers[GET_Cb(*pc)];
                _brh
            GTE:
                registers[i64cmt]=registers[GET_Ca(*pc)]>=registers[GET_Cb(*pc)];
                _brh
            MOVL: // tested
                o2 = &(fp+GET_Da(*pc))->object;
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
                o2 = &((sp+GET_Da(*pc))->object);
                _brh
            MOVF: // TODO: change this in the JIT
                registers[GET_Da(*pc)]=env->floatingPoints[*(pc + 1)]
                _brh_inc(2)
            SIZEOF:
                if(o2==NULL || o2->object == NULL)
                    registers[GET_Da(*pc)] = 0;
                else
                    registers[GET_Da(*pc)]=o2->object->size;
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
                        if((registers[GET_Da(*pc)]<o2->object->size) &&!(registers[GET_Da(*pc)]<0)) { _brh }
                        else {
                            stringstream ss;
                            ss << "Access to Object at: " << registers[GET_Da(*pc)] << " size is " << o2->object->size;
                            throw Exception(Environment::IndexOutOfBoundsException, ss.str());
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
                (++sp)->object = o2;
                STACK_CHECK _brh
            DEL:
            GarbageCollector::self->releaseObject(o2);
                _brh
            CALL:
#ifdef SHARP_PROF_
            tprof->hit(env->methods+GET_Da(*pc));
#endif
                CALLSTACK_CHECK
                if((jitFn = executeMethod(GET_Da(*pc), this)) != NULL) {

#ifdef BUILD_JIT
                    jitFn(jctx);
#endif
                }
                THREAD_EXECEPT();
                _brh_NOINCREMENT
            CALLD:
#ifdef SHARP_PROF_
            tprof->hit(env->methods+GET_Da(*pc));
#endif
                if((val = (int64_t )registers[GET_Da(*pc)]) <= 0 || val >= manifest.methods) {
                    stringstream ss;
                    ss << "invalid call to pointer of " << val;
                    throw Exception(ss.str());
                }
                CALLSTACK_CHECK
                if((jitFn = executeMethod(val, this)) != NULL) {

#ifdef BUILD_JIT
                    jitFn(jctx);
#endif
                }
                THREAD_EXECEPT();
                _brh_NOINCREMENT
            NEWCLASS:
                (++sp)->object =
                        GarbageCollector::self->newObject(&env->classes[GET_Da(*pc)]);
                STACK_CHECK _brh
            MOVN:
                CHECK_NULLOBJ(
                        if(GET_Da(*pc) >= o2->object->size || GET_Da(*pc) < 0)
                            throw Exception("movn");

                        o2 = &o2->object->node[GET_Da(*pc)];
                )
                _brh
            SLEEP:
                __os_sleep((int64_t)registers[GET_Da(*pc)]);
                _brh
            TEST:
                registers[i64cmt]=registers[GET_Ca(*pc)]==registers[GET_Cb(*pc)];
                _brh
            TNE:
                registers[i64cmt]=registers[GET_Ca(*pc)]!=registers[GET_Cb(*pc)];
                _brh
            LOCK:
                CHECK_NULL2(Object::monitorLock(o2, thread_self);)
                _brh
            ULOCK:
                CHECK_NULL2(Object::monitorUnLock(o2, thread_self);)
                _brh
            MOVG:
                o2 = env->globalHeap+GET_Da(*pc);
                _brh
            MOVND:
                CHECK_NULLOBJ(o2 = &o2->object->node[(int64_t)registers[GET_Da(*pc)]];)
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
                registers[*(pc+1)]=(int64_t)registers[GET_Ca(*pc)]<<(int64_t)registers[GET_Cb(*pc)];
                _brh_inc(2)
            SHR:
                registers[*(pc+1)]=(int64_t)registers[GET_Ca(*pc)]>>(int64_t)registers[GET_Cb(*pc)];
                _brh_inc(2)
            SKPE:
                LONG_CALL();
                if(registers[i64cmt]) {
                    pc = pc+GET_Da(*pc); _brh_NOINCREMENT
                } else _brh
            SKNE:
                LONG_CALL();
                if(registers[i64cmt]==0) {
                    pc = pc+GET_Da(*pc); _brh_NOINCREMENT
                } else _brh
            CMP:
                registers[i64cmt]=registers[GET_Ca(*pc)]==GET_Cb(*pc);
                _brh
            AND:
                registers[i64cmt]=registers[GET_Ca(*pc)]&&registers[GET_Cb(*pc)];
                _brh
            UAND:
                registers[i64cmt]=(int64_t)registers[GET_Ca(*pc)]&(int64_t)registers[GET_Cb(*pc)];
                _brh
            OR:
                registers[i64cmt]=(int64_t)registers[GET_Ca(*pc)]|(int64_t)registers[GET_Cb(*pc)];
                _brh
            XOR:
                registers[i64cmt]=(int64_t)registers[GET_Ca(*pc)]^(int64_t)registers[GET_Cb(*pc)];
                _brh
            THROW:
                exceptionObject = sp->object;
                throw Exception("", false);
                _brh
            CHECKNULL:
                registers[i64cmt]=o2 == NULL || o2->object==NULL;
                _brh
            RETURNOBJ:
                fp->object=o2;
                _brh
            NEWCLASSARRAY:
                (++sp)->object = GarbageCollector::self->newObjectArray(registers[GET_Ca(*pc)],
                                                                    &env->classes[GET_Cb(*pc)]);
                STACK_CHECK _brh
            NEWSTRING:
                GarbageCollector::self->createStringArray(&(++sp)->object, env->getStringById(GET_Da(*pc)));
                STACK_CHECK _brh
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
                (fp+GET_Cb(*pc))->var+=GET_Ca(*pc);
                _brh
            ISUBL:
                (fp+GET_Cb(*pc))->var-=GET_Ca(*pc);
                _brh
            IMULL:
                (fp+GET_Cb(*pc))->var*=GET_Ca(*pc);
                _brh
            IDIVL:
                (fp+GET_Cb(*pc))->var/=GET_Ca(*pc);
                _brh
            IMODL:
                val = (fp+GET_Cb(*pc))->var;
                (fp+GET_Cb(*pc))->var=val%GET_Ca(*pc);
                _brh
            LOADL:
                registers[GET_Ca(*pc)]=(fp+GET_Cb(*pc))->var;
                _brh
            IALOAD:
                CHECK_INULLOBJ(
                        registers[GET_Ca(*pc)] = o2->object->HEAD[(int64_t)registers[GET_Cb(*pc)]];
                )
                _brh
            POPOBJ:
                CHECK_NULL(
                        *o2 = (sp--)->object;
                )
                _brh
            SMOVR:
                (sp+GET_Cb(*pc))->var=registers[GET_Ca(*pc)];
                _brh
            SMOVR_2:
                (fp+GET_Cb(*pc))->var=registers[GET_Ca(*pc)];
                _brh
            SMOVR_3:
                (fp+GET_Da(*pc))->object=o2;
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
                CHECK_INULLOBJ(
                        o2->object->HEAD[(int64_t)registers[GET_Ca(*pc)]]=registers[GET_Cb(*pc)];
                )
                _brh
            NEG:
                registers[GET_Ca(*pc)]=-registers[GET_Cb(*pc)]];
                _brh
            SMOV:
                registers[GET_Ca(*pc)]=(sp+GET_Cb(*pc))->var;
                _brh
            RETURNVAL:
                (fp)->var=registers[GET_Da(*pc)];
                _brh
            ISTORE:
                (++sp)->var = GET_Da(*pc);
                STACK_CHECK _brh
            ISTOREL:
                (fp+GET_Da(*pc))->var=*(pc+1);
                _brh_inc(2)
            PUSHNULL:
            GarbageCollector::self->releaseObject(&(++sp)->object);
                STACK_CHECK _brh
            IPUSHL:
                (++sp)->var = (fp+GET_Da(*pc))->var;
                STACK_CHECK _brh
            PUSHL:
                (++sp)->object = (fp+GET_Da(*pc))->object;
                STACK_CHECK _brh
            ITEST:
                Object *obj = &(sp--)->object;
                registers[GET_Da(*pc)] = obj->object == (sp--)->object.object;
                _brh
            INVOKE_DELEGATE:
                invokeDelegate(GET_Ca(*pc), GET_Cb(*pc), this, 0);
                THREAD_EXECEPT();
                _brh_NOINCREMENT
            ISADD:
                (sp+GET_Cb(*pc))->var+=GET_Ca(*pc);
                _brh
            JE:
                LONG_CALL();
                if(registers[i64cmt]) {
                    pc=cache+GET_Da(*pc); _brh_NOINCREMENT
                } else  _brh
            JNE:
                LONG_CALL();
                if(registers[i64cmt]==0) {
                    pc=cache+GET_Da(*pc); _brh_NOINCREMENT
                } else  _brh
            SWITCH: {
                LONG_CALL();
                executeSwitch(this, GET_Da(*pc));
                _brh_NOINCREMENT
            }
            TLS_MOVL:
                o2 = &(dataStack+GET_Da(*pc))->object;
                _brh
            DUP:
                obj = &sp->object;
                (++sp)->object = obj;
                _brh
            POPOBJ_2:
                o2 = &(sp--)->object;
                _brh
            SWAP:
                if((sp-dataStack) >= 2) {
                    o = sp->object.object;
                    (sp)->object = (sp-1)->object;
                    (sp-1)->object = o;
                } else {
                    stringstream ss;
                    ss << "Illegal stack swap while sp is ( " << (x86int_t )(sp - dataStack) << ") ";
                    throw Exception(Environment::ThreadStackException, ss.str());
                }
                _brh
            EXP:
                registers[GET_Ba(*pc)]=pow(registers[GET_Bb(*pc)], registers[GET_Bc(*pc)]);
                _brh



        }
    } catch (bad_alloc &e) {
        cout << "std::bad_alloc\n";
        // TODO: throw out of memory error
    } catch (Exception &e) {
        throwable = e.getThrowable();
    }

    exception_catch:
    if(state == THREAD_KILLED) {
        sendSignal(thread_self->signal, tsig_except, 1);
        vm->fillStackTrace(throwable.stackTrace);
        return;
    }
    vm->Throw();

    /**
     * Exception is still live and must make its transition to low
     * level Sharp
     */
    if(hasSignal(signal, tsig_except) || current->isjit) {
        return;
    }

    DISPATCH();
}

void Thread::setup() {
    current = NULL;
    calls=-1;
    signal=tsig_empty;
    suspended = false;
    exited = false;
    terminated = false;
    exitVal = 0;
    starting = 0;
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
        int priority = (int)env->__sgetFieldVar("priority", currentThread.object);
        setPriority(this, priority);

        if(currentThread.object != nullptr
           && IS_CLASS(currentThread.object->info)) {
            Object *threadName = env->findField("name", currentThread.object);

            if(threadName != NULL) { // reset thread name
                env->createString(threadName, name);
            }
        }
        fp=&dataStack[manifest.threadLocals];
        sp=(&dataStack[manifest.threadLocals])-1;

        for(unsigned long i = 0; i < stackLimit; i++) {
            this->dataStack[i].object.object = NULL;
            this->dataStack[i].var=0;
        }
    } else {
        GarbageCollector::self->addMemory(sizeof(StackElement) * stackLimit);
        setupSigHandler();
    }
}

int Thread::setPriority(Thread* thread, int priority) {
    if(thread->thread != NULL) {
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
        }

        pthread_setschedprio(thread->thread, pthread_prio);
        pthread_attr_destroy(&thAttr);
#endif
    }

    return RESULT_ILL_PRIORITY_SET;
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
    std::lock_guard<std::mutex> guard(threadsMonitor);

    Thread *thread;
    boolean wrapped = false;

    do {
        tid++;
        if(tid < 0) {
            if(wrapped)
                return ILL_THREAD_ID;
            else { wrapped = true; tid = jit_threadid; }
        }
    }
    while(threads.get(tid, thread));

    if(tid > maxThreadId)
        maxThreadId = tid;
    return tid;
}

void Thread::init(string name, Int id, Method *main, bool daemon, bool initializeStack)  {
    Thread();
    this->name = name;
    this->id = id;
    this->main = main;
    this->stackLimit = internalStackSize;
    this->daemon=daemon;
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

void __os_sleep(int64_t INTERVAL) {
#ifdef WIN32_
    Sleep(INTERVAL);
#endif
#ifdef POSIX_
    usleep(INTERVAL);
#endif
}
