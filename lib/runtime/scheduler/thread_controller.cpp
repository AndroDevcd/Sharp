//
// Created by bnunnally on 8/17/21.
//

#include "thread_controller.h"
#include "scheduler.h"
#include "task_controller.h"
#include "thread_exception_controller.h"
#include "../Manifest.h"
#include "../VirtualMachine.h"
#include "../Thread.h"

uInt lastThreadId = -1;
Thread *mainThread = NULL;

bool valid_stack_size(uInt stackSize) {
    return stackSize >= STACK_MIN;
}

bool valid_internal_stack_size(uInt stackSize) {
    return stackSize >= INTERNAL_STACK_MIN;
}

void create_main_thread() {
    Thread* main = (Thread*)malloc(
            sizeof(Thread)*1);

    mainThread = main;
    string threadName = "Main";
    uInt threadId = generate_thread_id();
    if(threadId == ILL_THREAD_ID)
        return;

    initialize(main, threadName, threadId, false, vm.getMainMethod(), true);
    main->priority = THREAD_PRIORITY_HIGH;

    post(main);
}

Thread* get_main_thread() {
    return mainThread;
}

Thread* create_gc_thread() {
    Thread* gc = (Thread*)malloc(
            sizeof(Thread)*1);

    string threadName = "Gc";
    uInt threadId = generate_thread_id();
    if(threadId == ILL_THREAD_ID)
        return NULL; // unlikely

    initialize(gc, threadName, threadId, true, NULL, false);
    gc->priority = THREAD_PRIORITY_LOW;

    post(gc);
    return gc;
}

/**
 * We must use sendSignal() synanomyously with interrupt and other calls
 * that may kill a thread because this bit flag is used by the JIT Runtime
 * @param thread
 * @return
 */
uInt interrupt_thread(Thread* thread) {
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
            send_interrupt_signal(thread);
        }

        return RESULT_OK;
    }

    return RESULT_THREAD_INTERRUPT_FAILED;
}

void send_interrupt_signal(Thread* thread) {
    GUARD(thread->mutex);
    sendSignal(thread->signal, tsig_kill, 1);
}

void send_suspend_signal(Thread* thread) {
    if(thread_self && thread->id == thread_self->id)
        suspend_self();
    else {
        GUARD(thread->mutex);
        sendSignal(thread->signal, tsig_suspend, 1);
    }
}

uInt send_unsuspend_signal(Thread* thread) {
    GUARD(thread->mutex);
    thread->suspended = false;
    sendSignal(thread->signal, tsig_suspend, 0);
    return RESULT_OK;
}

uInt suspend_thread(uInt id) {
    Thread *thread = get_thread(id);

    if (thread_self != NULL && (thread == NULL || thread->state != THREAD_RUNNING))
        return RESULT_ILL_THREAD_SUSPEND;

    if(thread->terminated)
        return RESULT_THREAD_TERMINATED;

    send_suspend_signal(thread);
    return RESULT_OK;
}

uInt join_thread(Thread* thread) {
    if(thread->state == THREAD_STARTED)
        wait_for_thread_start(thread);

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

uInt join_thread(uInt id) {
    if (thread_self != NULL && (id == thread_self->id ))
        return RESULT_ILL_THREAD_JOIN;

    Thread* thread = get_thread(id);
    if (thread == NULL)
        return RESULT_ILL_THREAD_JOIN;
    if(thread->terminated)
        return RESULT_THREAD_TERMINATED;

    return join_thread(thread);
}

uInt interrupt_thread(uInt id) {
    if(id == thread_self->id)
        return RESULT_ILL_THREAD_INTERRUPT; // cannot interrupt thread_self

    Thread* thread = get_thread(id);
    if(thread == NULL || thread->daemon)
        return RESULT_THREAD_INTERRUPT_FAILED;
    if(thread->terminated)
        return RESULT_THREAD_TERMINATED;

    int result = interrupt_thread(thread);
    if(result == RESULT_OK && vm.state != VM_TERMINATED)
        wait_for_thread_exit(thread);
    return result;
}

uInt unsuspend_thread(uInt id, bool wait) {
    Thread *thread = get_thread(id);

    if (thread_self != NULL && (thread == NULL || (!wait && thread->state != THREAD_SUSPENDED)))
        return RESULT_ILL_THREAD_SUSPEND;

    if(thread->terminated)
        return RESULT_THREAD_TERMINATED;

    if(wait) {
        unsuspend_and_wait(thread, true);
        return RESULT_OK;
    }

    return send_unsuspend_signal(thread);
}

void cancel_context_switch(Thread *thread) {
    GUARD(mutex)
    thread->contextSwitching = false;
    sendSignal(thread->signal, tsig_context_switch, 0);
#ifdef COROUTINE_DEBUGGING
    thread->skipped++;
#endif
}

int set_thread_priority(uInt id, int priority) {
    Thread* thread = get_thread(id);
    if(thread == NULL || thread->terminated || thread->state==THREAD_KILLED
       || thread->state==THREAD_CREATED)
        return RESULT_ILL_PRIORITY_SET;

    return set_thread_priority(thread, priority);
}

int set_thread_priority(Thread* thread, int priority) {
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

void cancel_task(Thread *thread) {
    GUARD(thread->mutex)
    thread->newTask = NULL;
}

void wait_for_context_switch(Thread* thread) {
    const int sMaxRetries = 1000000;
    int retryCount = 0;
    int retryLimit = 10;

    while (!thread->waiting)
    {
        if (retryCount++ == sMaxRetries)
        {
            retryLimit--;
            retryCount = 0;
            if (vm.state >= VM_SHUTTING_DOWN
                || thread->state == THREAD_KILLED
                || hasSignal(thread->signal, tsig_kill)) {
                break;
            }

            __usleep(1);
            if(retryLimit <= 0) {
                break;
            }
        }
    }
}

void post_task(Thread* thread, fiber *newTask) {
    if(thread->newTask == NULL && thread->waiting) {
        thread->newTask = newTask;
        const int sMaxRetries = 1000000;
        int retryCount = 0;
        int retryLimit = 10;

        while (thread->newTask != NULL)
        {
            if (retryCount++ == sMaxRetries)
            {
                retryLimit--;
                retryCount = 0;
                if (vm.state >= VM_SHUTTING_DOWN
                    || thread->state == THREAD_KILLED
                    || hasSignal(thread->signal, tsig_kill)) {
                    cancel_task(thread);
                    break;
                }

                __usleep(1);
                if(retryLimit <= 0) {
                    cancel_task(thread);
                    break;
                }
            }
        }
    }
}

bool accept_task(Thread* thread) {
    GUARD(thread->mutex)
    if(thread->newTask != NULL) {
        thread->this_fiber = thread->newTask;
        set_task_state(thread, thread->this_fiber, FIB_RUNNING, NO_DELAY);
        thread->waiting = false;
        thread->newTask = NULL;
        return true;
    }

#ifdef COROUTINE_DEBUGGING
    thread->skipped++;
#endif
    return false;
}

void wait_for_posted_task(Thread* thread) {

    fiber *this_fiber = thread->this_fiber;
#ifdef COROUTINE_DEBUGGING
    thread->switched++;
    Int start = NANO_TOMICRO(Clock::realTimeInNSecs());
#endif
    set_attached_thread(this_fiber, NULL);

    if(this_fiber->finished) {
        if(thread->boundFibers > 1 || this_fiber->boundThread != thread) {
            set_task_state(NULL, this_fiber, FIB_KILLED, NO_DELAY);
        }
        else {
            thread->waiting = false;

#ifdef COROUTINE_DEBUGGING
            thread->switched++;
            thread->contextSwitchTime += NANO_TOMICRO(Clock::realTimeInNSecs()) - start;
#endif
            return;
        }
    } else if(this_fiber->state == FIB_RUNNING) {
        set_task_state(thread, this_fiber, FIB_SUSPENDED, NO_DELAY);
    }

    this_fiber=NULL;
    thread->waiting = true;
    thread->contextSwitching = false;
    enable_context_switch(thread, false);

    while(true)
    {
        if(thread->newTask != NULL
            && accept_task(thread)) {
#ifdef COROUTINE_DEBUGGING
            thread->switched++;
            thread->contextSwitchTime += NANO_TOMICRO(Clock::realTimeInNSecs()) - start;
#endif
            break;
        }

#ifdef COROUTINE_DEBUGGING
        thread->actualSleepTime+=1;
#endif
        __usleep(1);
        if (hasSignal(thread->signal, tsig_suspend))
            suspend_self();
        if (thread->state == THREAD_KILLED || hasSignal(thread->signal, tsig_kill))
            return;
    }
}

void enable_context_switch(Thread* thread, bool enable) {
    GUARD(mutex);
    sendSignal(thread->signal, tsig_context_switch, (enable ? 1 : 0));
}

bool try_context_switch(Thread *thread, bool incPc) {
    if(thread->contextSwitching) return true;
    if(thread->this_fiber == NULL || thread->this_fiber->callStack == NULL
       || thread->this_fiber->current->nativeFunc)  {
        cancel_context_switch(thread);
        return false;
    }

    for(Int i = 0; i < thread->this_fiber->calls; i++) {
        Frame &frame = thread->this_fiber->callStack[i];
        if(vm.methods[frame.returnAddress].nativeFunc) {
            cancel_context_switch(thread);
            return false;
        }
    }

    if(incPc) thread->this_fiber->pc++;
    thread->contextSwitching = true;
    return true;
}

uInt start_thread(Thread* thread, size_t stackSize) {
    if (thread_self != NULL && (thread == NULL || thread->state == THREAD_STARTED))
        return RESULT_ILL_THREAD_START;

    if(thread->state == THREAD_RUNNING)
        return RESULT_THREAD_RUNNING;

    if(thread->terminated)
        return RESULT_THREAD_TERMINATED;

    if(stackSize && !valid_stack_size(stackSize))
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
    else return wait_for_thread_start(thread);
#endif
#ifdef POSIX_
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr,thread->stackSize + STACK_OVERFLOW_BUF);
    if(pthread_create( &thread->thread, &attr, VirtualMachine::InterpreterThreadStart, (void*) thread))
        return RESULT_THREAD_NOT_STARTED; // thread was not started
    else {
        return wait_for_thread_start(thread);
    }
#endif
}

uInt start_thread(uInt threadId, size_t stackSize) {
    Thread *thread = get_thread(threadId);
    return start_thread(thread, stackSize);
}


int start_daemon_thread(
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
        return wait_for_thread_start(thread);
#endif
#ifdef POSIX_
    if(pthread_create( &thread->thread, NULL, threadFunc, (void*) thread)!=0)
        return RESULT_THREAD_NOT_STARTED; // thread was not started
    else
        return wait_for_thread_start(thread);
#endif
}

void setup_thread(Thread* thread) {
    thread->signal=tsig_empty;
    thread->suspended = false;
    thread->exited = false;
    thread->terminated = false;

    if(thread->id != main_threadid){
        thread->this_fiber = create_task(thread->name, thread->mainMethod);
        bind_task(thread->this_fiber, thread);

        if(thread->currentThread.object != nullptr
           && IS_CLASS(thread->currentThread.object->info)) {
            gc.createStringArray(vm.resolveField("data",
                       vm.resolveField("name", thread->currentThread.object)->object), thread->name);
        }
    } else {
        vm.state = VM_RUNNING;
        setupSigHandler();
    }


#ifdef SHARP_PROF_
    tprof = new Profiler();
    tprof->init(this_fiber->stackLimit);
    tprof->starttm=Clock::realTimeInNSecs();
    for(size_t i = 0; i < vm.manifest.methods; i++) {
        funcProf prof = funcProf(vm.methods+i);
        tprof->functions.push_back(prof);
    }
#endif

    if(thread->currentThread.object != nullptr
       && CLASS(thread->currentThread.object->info) == vm.ThreadClass->address) {
        vm.initializeField("fib", thread->currentThread.object, vm.FiberClass);
        Object *fibField = vm.resolveField("fib", thread->currentThread.object);
        Object *threadMainField = vm.resolveField("main", thread->currentThread.object);
        Object *threadNameField = vm.resolveField("name", thread->currentThread.object);

        if(fibField && threadMainField && threadNameField) {
            if(thread->id == main_threadid) {
                thread->this_fiber->fiberObject = fibField;
            }

            Object *nameField = vm.resolveField("name", fibField->object);
            vm.setFieldVar("main", fibField->object, 0, vm.numberValue(0, threadMainField->object));
            vm.setFieldVar("id", fibField->object, 0, thread->this_fiber->id);

            if(nameField) {
                *nameField = threadNameField;
            }

            if(thread->this_fiber->fiberObject.object == NULL)
                thread->this_fiber->fiberObject = fibField;
        }
    }
}

void __os_sleep(uInt time) {
    if(time < 0) return;
    __usleep(time);

#ifdef COROUTINE_DEBUGGING
    if(thread_self)
       thread_self->actualSleepTime+=time;
#endif
}

void release_resources(Thread* thread) {
    gc.reconcileLocks(thread);
}

void shutdown_thread(Thread* thread) {
    GUARD(mutex);
    if(thread->id == main_threadid) {
        if (thread->this_fiber && thread->this_fiber->dataStack != NULL)
            thread->this_fiber->exitVal = (int) thread->this_fiber->dataStack[vm.manifest.threadLocals].var;
    }

    print_exception(thread);
    release_resources(thread);
    if(thread->id != main_threadid && thread->this_fiber) {
        set_task_state(NULL, thread->this_fiber, FIB_KILLED, NO_DELAY);
    }

    thread->signal = tsig_empty;
    thread->exited = true;
    thread->state = THREAD_KILLED;
}

void kill_all_threads() {
    GUARD(thread_mutex);
    suspend_all_threads(false);
    _sched_thread *scht = sched_threads;
    Thread *thread;

    while(scht != NULL)
    {
        thread = scht->thread;

#ifdef COROUTINE_DEBUGGING
        cout << "Thread " << thread->name << " slept: " << thread->timeSleeping << " switched: " << thread->switched
             << " bound: " << bound_task_count(thread) << " skipped " << thread->skipped << " actual sleep time: " << (thread->actualSleepTime / 1000)
             << " context switch time (ms) " << (thread->contextSwitchTime / 1000) << endl << " time spent locking: " << thread->timeLocking << endl;
#endif
        if(thread->id != thread_self->id
           && thread->state != THREAD_KILLED && thread->state != THREAD_CREATED) {
            interrupt_and_wait(thread);
        }

        scht = scht->next;
    }
}

void wait_for_thread_exit(Thread* thread) {
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
uInt wait_for_thread_start(Thread* thread) {
    const int sMaxRetries = 1000000;
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

void interrupt_and_wait(Thread* thread) {
    send_interrupt_signal(thread);
    wait_for_thread_exit(thread);
}

void suspend_and_wait(Thread* thread, bool sendSignal) {
    const int sMaxRetries = 1000000;
    const int sMaxSpinCount = 25; // TODO: test this extensivley to make sure there is no issues with lowering the threshold to giving up

    int retryCount = 0;

    if(sendSignal) send_suspend_signal(thread);
    while (thread->state == THREAD_RUNNING && !thread->suspended)
    {
        if (retryCount++ == sMaxRetries)
        {
            send_suspend_signal(thread);
            retryCount = 0;
            if(thread->state >= THREAD_SUSPENDED)
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

void unsuspend_and_wait(Thread* thread, bool sendSignal) {
    const int sMaxRetries = 1000000;
    const int sMaxSpinCount = 10;

    int spinCount = 0;
    int retryCount = 0;
    Thread *current = thread_self;

    if(sendSignal) send_unsuspend_signal(thread);
    while (thread->state != THREAD_RUNNING)
    {

        if (retryCount++ == sMaxRetries)
        {
            send_unsuspend_signal(thread);
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
            suspend_self();
        }
    }
}

void wait_for_unsuspend_release(Thread* thread) {
    const long sMaxRetries = 1000;

    long spinCount = 0;
    long retryCount = 0;

    thread->state = THREAD_SUSPENDED;

    while (thread->suspended)
    {
        if (retryCount++ == sMaxRetries)
        {
            spinCount++;
            retryCount = 0;

#ifdef COROUTINE_DEBUGGING
            thread->timeSleeping += 1000;
#endif
            __usleep(1000);
        } else if(thread->state == THREAD_KILLED) {
            thread->suspended = false;
            return;
        } else if(vm.state == VM_SHUTTING_DOWN) {
            thread->suspended = false;
            sendSignal(thread->signal, tsig_kill, 1);
            return;
        }
    }

    {
        GUARD(mutex);
        thread->state = THREAD_RUNNING;
        sendSignal(thread->signal, tsig_suspend, 0);
    }
}

void resume_all_threads(bool withMarking) {
    GUARD(thread_mutex)
    _sched_thread *scht = sched_threads;
    Thread *thread;

    while(scht != NULL)
    {
        thread = scht->thread;
        if(thread_self && thread->id != thread_self->id) {
            continue;
        }

        if(withMarking && thread->marked) {
            thread->marked = false;
        } else {
            unsuspend_and_wait(thread, true);
        }

        scht = scht->next;
    }
}

void suspend_all_threads(bool withMarking) {
    GUARD(thread_mutex)
    _sched_thread *scht = sched_threads;
    Thread *thread;

    while(scht != NULL)
    {
        thread = scht->thread;
        if(thread_self && thread->id != thread_self->id){
            scht = scht->next;
            continue;
        }

        if(thread->state == THREAD_RUNNING)
            suspend_and_wait(thread, true);
        else if(withMarking && thread->state == THREAD_SUSPENDED) {
            thread->marked = true;
        }

        scht = scht->next;
    }
}

void suspend_self() {
    Thread *thread = thread_self;

    if(thread)
    {
        thread->suspended = true;
        {
            GUARD(thread->mutex);
            sendSignal(thread->signal, tsig_suspend, 0);
        }

        /*
         * We call wait upon suspend. This function will
         * sleep the thread most of the time. unsuspendThread() or
         * resumeAllThreads() should be called to revive thread.
         */
        wait_for_unsuspend_release(thread);
    }
}

Thread* get_thread(uInt id) {
    if(vm.state >= VM_SHUTTING_DOWN) {
        return NULL;
    }

    GUARD(thread_mutex)
    _sched_thread *thread = sched_threads;

    while(thread != NULL)
    {
        if(thread->thread->id == id) return thread->thread;
        thread = thread->next;
    }

    for(uInt i = 0; i < unSchedThreads.size(); i++) {
        if(unSchedThreads.get(i)->id == id)
            return unSchedThreads.get(i);
    }

    return NULL;
}

uInt create_thread(uInt methodAddr, bool daemon) {
    int32_t threadId = generate_thread_id();
    registers[CMT] = false;
    if(methodAddr < 0 || methodAddr >= vm.manifest.methods)
        return RESULT_THREAD_CREATE_FAILED;

    if(threadId == ILL_THREAD_ID)
        return RESULT_NO_THREAD_ID;

    Method* method = &vm.methods[methodAddr];
    if(method->paramSize!=0 || method->fnType == fn_ptr)
        return RESULT_THREAD_CREATE_FAILED;

    Thread* thread = (Thread*)__malloc(sizeof(Thread));

    stringstream ss;
    ss << "Thread@" << threadId;
    string name = ss.str();
    initialize(thread, name, threadId, daemon, method, true);

    post(thread);
    registers[CMT] = true;
    return threadId;
}

void initialize(Thread *thread, string &name, uInt id, bool daemon, Method* main, bool initializeStack) {
    thread->init();
    thread->name = name;
    thread->id = id;
    thread->daemon=daemon;
    thread->mainMethod=main;
    if(initializeStack) {
        thread->this_fiber = create_task(thread->name, main);
        bind_task(thread->this_fiber, thread);
    }
}

bool is_thread_id_available(uInt threadId) {
    GUARD(thread_mutex)
    _sched_thread *thread = sched_threads;

    while(thread != NULL)
    {
        if(thread->thread->id == threadId) return false;
        thread = thread->next;
    }

    return true;
}

uInt generate_thread_id() {
    bool wrapped = false;

    while(!is_thread_id_available(++lastThreadId)) {
        if(lastThreadId == ((uInt)-1)) {
            if(wrapped)
                return ILL_THREAD_ID;
            else { wrapped = true; lastThreadId = idle_sched_threadid; }
        }
    }

    return lastThreadId;
}

bool can_dispose(_sched_thread *scht) {
    return vm.state != VM_SHUTTING_DOWN && scht->thread->state == THREAD_KILLED;
}

void dispose(_sched_thread *scht) {
    kill_bound_fibers(scht->thread);

    GUARD(thread_mutex)
    Thread *thread = scht->thread;
    if (thread->state == THREAD_KILLED || thread->terminated)
    {
        if(scht == last_thread)
            last_thread = scht->prev;
        if(scht->prev != NULL)
            scht->prev->next = scht->next;
        if(scht->next != NULL)
            scht->next->prev = scht->prev;

        thread->free();
        std::free (thread);
        std::free (scht);
    }
}

/**
 * Every thread is not guaranteed to be scheduled every time.
 * We will only schedule a thread if it has ra past its allotted time slice
 * and the thread is waiting to recieve another task. Threads can ignore this
 * request however for amy reason and continue to execute the same task that they are running.
 *
 * @param scht The current thread being scheduled a task
 * @return The next thread to be sched.
 */
_sched_thread* sched_thread(_sched_thread *scht) {
    _sched_thread *next_thread = scht->next;
    if (can_dispose(scht)) {
        dispose(scht);
        return next_thread;
    } else if(scht->thread->state != THREAD_RUNNING)
        return next_thread;

    if (!scht->thread->waiting && is_thread_ready(scht->thread)) {
        enable_context_switch(scht->thread, true);
        __os_yield();
//        wait_for_context_switch(scht->thread); // todo: try having an impaitent scheduler
    }

    if(scht->thread->waiting)
        sched(scht->thread);

    return next_thread;
}