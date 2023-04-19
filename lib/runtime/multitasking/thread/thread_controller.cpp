//
// Created by bknun on 9/19/2022.
//

#include "thread_controller.h"
#include "stack_limits.h"
#include "sharp_thread.h"
#include "../../memory/memory_helpers.h"
#include "../../../core/thread_state.h"
#include "../../vm_initializer.h"
#include "../../virtual_machine.h"
#include "../fiber/fiber.h"
#include "../fiber/task_controller.h"
#include "../../../util/time.h"
#include "../../sig_handler.h"

uInt lastThreadId = -1;
sharp_thread *mainThread = NULL;
thread_local sharp_thread *thread_self = NULL;

void create_main_thread() {
    sharp_thread* main = (sharp_thread*)malloc(
            sizeof(sharp_thread));

    mainThread = main;
    string threadName = "Main";
    uInt threadId = new_thread_id();
    if(threadId == ILL_THREAD_ID)
        return;

    setup_thread(main, threadName, threadId, false, get_main_method(), true);
    main->priority = THREAD_PRIORITY_HIGH;

    post(main);
}

sharp_thread* get_main_thread() {
    return mainThread;
}

sharp_function* get_main_method() {
    return vm.methods + vm.manifest.entryMethod;
}

#ifdef WIN32_
DWORD WINAPI
#endif
#ifdef POSIX_
void*
#endif
vm_thread_entry(void *arg)
{

}

sharp_thread* create_gc_thread() {
    sharp_thread* gc_thread = (sharp_thread*)malloc(
            sizeof(sharp_thread));

    string threadName = "Gc";
    uInt threadId = new_thread_id();
    if(threadId == ILL_THREAD_ID)
        return NULL; // unlikely

    setup_thread(gc_thread, threadName, threadId, true, nullptr, false);
    gc_thread->priority = THREAD_PRIORITY_LOW;

    post(gc_thread);
    return gc_thread;
}

/**
 * We must use sendSignal() synanomyously with interrupt and other calls
 * that may kill a thread because this bit flag is used by the JIT Runtime
 * @param thread
 * @return
 */
uInt interrupt_thread(sharp_thread* thread) {
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
            shutdown();
        }
        else
        {
            send_interrupt_signal(thread);
        }

        return RESULT_OK;
    }

    return RESULT_THREAD_INTERRUPT_FAILED;
}

void send_interrupt_signal(sharp_thread* thread) {
    guard_mutex(thread->mut);
    sendSignal(thread->signal, tsig_kill, 1);
}

void send_suspend_signal(sharp_thread* thread) {
    if(thread_self && thread->id == thread_self->id)
        suspend_self();
    else {
        guard_mutex(thread->mut);
        sendSignal(thread->signal, tsig_suspend, 1);
    }
}

uInt send_unsuspend_signal(sharp_thread* thread) {
    guard_mutex(thread->mut);
    thread->suspended = false;
    sendSignal(thread->signal, tsig_suspend, 0);
    return RESULT_OK;
}

uInt suspend_thread(uInt id) {
    sharp_thread *thread = get_thread(id);

    if (thread_self != NULL && (thread == NULL || thread->state != THREAD_RUNNING))
        return RESULT_ILL_THREAD_SUSPEND;

    if(thread->terminated)
        return RESULT_THREAD_TERMINATED;

    send_suspend_signal(thread);
    return RESULT_OK;
}

uInt join_thread(sharp_thread* thread) {
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

    sharp_thread* thread = get_thread(id);
    if (thread == NULL)
        return RESULT_ILL_THREAD_JOIN;
    if(thread->terminated)
        return RESULT_THREAD_TERMINATED;

    return join_thread(thread);
}

uInt interrupt_thread(uInt id) {
    if(id == thread_self->id)
        return RESULT_ILL_THREAD_INTERRUPT; // cannot interrupt thread_self

    sharp_thread* thread = get_thread(id);
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
    sharp_thread *thread = get_thread(id);

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

int set_thread_priority(uInt id, int priority) {
    sharp_thread* thread = get_thread(id);
    if(thread == NULL || thread->terminated || thread->state==THREAD_KILLED
       || thread->state==THREAD_CREATED)
        return RESULT_ILL_PRIORITY_SET;

    return set_thread_priority(thread, priority);
}

int set_thread_priority(sharp_thread* thread, int priority) {
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

void post_task(sharp_thread* thread, sched_task *newTask) {
    try_handshake(&thread->hs, newTask, HANDSHAKE_NEW_TASK, CLOCK_CYCLE / threadCount);
}

void wait_for_posted_task(sharp_thread* thread) {
    fiber *this_fiber = thread->scht->task;
#ifdef COROUTINE_DEBUGGING
    thread->switched++;
    Int start = NANO_TOMICRO(Clock::realTimeInNSecs());
#endif

    if(this_fiber->finished) {
        if(thread->boundFibers > 1 || this_fiber->boundThread != thread) {
            dispose(thread->scht);
        }
        else {
#ifdef COROUTINE_DEBUGGING
            thread->switched++;
            thread->contextSwitchTime += NANO_TOMICRO(Clock::realTimeInNSecs()) - start;
#endif
            return;
        }
    } else {
        set_attached_thread(this_fiber, NULL);

        if(this_fiber->state == FIB_RUNNING) {
            set_task_state(thread, this_fiber, FIB_SUSPENDED, NO_DELAY);
        }
    }

    while (true) {
        auto data = listen(&thread->hs);

        if(data.key == HANDSHAKE_NEW_TASK) {
            thread->scht = (sched_task*) data.value;
            set_task_state(thread, thread->scht->task, FIB_RUNNING, NO_DELAY);
            break;
        }
    }
}

void enable_context_switch(sharp_thread* thread, bool enable) {
    guard_mutex(mutex);
    sendSignal(thread->signal, tsig_context_switch, (enable ? 1 : 0));
}

uInt start_thread(sharp_thread* thread, size_t stackSize) {
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
            vm_thread_entry,  // thread function caller
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
    if(pthread_create( &thread->thread, &attr, vm_thread_entry, (void*) thread))
        return RESULT_THREAD_NOT_STARTED; // thread was not started
    else {
        return wait_for_thread_start(thread);
    }
#endif
}

uInt start_thread(uInt threadId, size_t stackSize) {
    sharp_thread *thread = get_thread(threadId);
    return start_thread(thread, stackSize);
}


int start_daemon_thread(
#ifdef WIN32_
        DWORD WINAPI
#endif
#ifdef POSIX_
void*
#endif
(*threadFunc)(void *), sharp_thread *thread) {
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

void setup_thread(sharp_thread* thread) {
    thread->signal=tsig_empty;
    thread->suspended = false;
    thread->exited = false;
    thread->terminated = false;

    if(thread->id != main_threadid){
        auto task = create_task(thread->name, thread->mainMethod);
        bind_task(task, thread);
        set_context_state(thread, THREAD_ACCEPTING_TASKS);

        if(thread->currentThread.o != nullptr
           && IS_CLASS(thread->currentThread.o->info)) {
            gc.createStringArray(vm.resolveField("data",
                                                 vm.resolveField("name", thread->currentThread.object)->object), thread->name);
        }
    } else {
        vm.state = VM_RUNNING;
        setup_sig_handler();
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

void release_resources(sharp_thread* thread) {
    gc.reconcileLocks(thread);
}

void shutdown_thread(sharp_thread* thread) {
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

void wait_for_thread_exit(sharp_thread* thread) {
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
uInt wait_for_thread_start(sharp_thread* thread) {
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

void interrupt_and_wait(sharp_thread* thread) {
    send_interrupt_signal(thread);
    wait_for_thread_exit(thread);
}

void suspend_and_wait(sharp_thread* thread, bool sendSignal) {
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

void unsuspend_and_wait(sharp_thread* thread, bool sendSignal) {
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

void wait_for_unsuspend_release(sharp_thread* thread) {
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

sharp_thread* get_thread(uInt id) {
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

    _unsched_thread *unsched_t = unsched_threads;

    while(unsched_t != NULL)
    {
        if(unsched_t->thread->id == id) return unsched_t->thread;
        unsched_t = unsched_t->next;
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

    sharp_thread* thread = (sharp_thread*)__malloc(sizeof(Thread));

    stringstream ss;
    ss << "Thread@" << threadId;
    string name = ss.str();
    setup_thread(thread, name, threadId, daemon, method, true);

    post(thread);
    registers[CMT] = true;
    return threadId;
}

void setup_thread(sharp_thread *thread, string &name, uInt id, bool daemon, sharp_function* main, bool initializeStack) {
    thread->name = name;
    thread->id = id;
    thread->daemon=daemon;
    thread->mainMethod=main;
    if(initializeStack) {
        auto task = create_task(thread->name, main);
        bind_task(task, thread);
        set_context_state(thread, THREAD_ACCEPTING_TASKS);
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
 * @return The next thread to be
 * .
 */
_sched_thread* sched_thread(_sched_thread *scht) {
    _sched_thread *next_thread = scht->next;
    if (can_dispose(scht)) {
        dispose(scht);
        return next_thread;
    } else if(scht->thread->state != THREAD_RUNNING)
        return next_thread;

    if (is_thread_ready(scht->thread)) {
        enable_context_switch(scht->thread, true);
        _usleep(1)
    }

    if(scht->thread->waiting)
        sched(scht->thread);

    return next_thread;
}
