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
#include "../../error/vm_exception.h"
#include "../../reflect/reflect_helpers.h"
#include "../scheduler/idle_scheduler.h"
#include "../../main.h"
#include "../../telemetry/telemetry.h"

uInt lastThreadId = -1;
sharp_thread *mainThread = NULL;
thread_local sharp_thread *thread_self = NULL;

void create_main_thread() {
    sharp_thread* main = (sharp_thread*)malloc(
            sizeof(sharp_thread));

    init_struct(main);
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
    return vm.methods + vm.mf.entryMethod;
}

bool all_tasks_finished(sharp_thread *thread) {
    return thread->boundFibers == 0
       || (thread->boundFibers == 1 && thread->task && thread->task->finished && thread->task->boundThread == thread);
}

#ifdef WIN32_
DWORD WINAPI
#endif
#ifdef POSIX_
void*
#endif
vm_thread_entry(void *arg)
{
    bool unboundException = false;
    sharp_thread *thread = (sharp_thread*)arg;
    thread_self = thread;
    TELEMETRY(telemetry_thread_begin(thread);)

    set_thread_priority(thread, thread->priority);

    start:
    try
    {
        if(unboundException)
            goto _unboundExceptionThrown;

        pop_queue(thread);
        setup_thread(thread);

        do {
            registers = thread->task->registers;
            if(thread->task->calls == 0) {
                prepare_method(thread->task->main->address);
            }

            // A fiber that already has callers on its stack is being continued:
            // its frames are rebuilt from the bottom (see next_rebuild_frame()).
            thread->task->rebuildDepth = 0;
            thread->task->rebuilding = thread->task->callFramePtr >= 0;

            main_vm_loop(thread->task->main);

            _unboundExceptionThrown:
            if(hasSignal(thread->signal, tsig_except)) {

                if(thread->task->boundThread != thread) {
                    /**
                     * Unbound fibers are more forgiving than bound fibers are.
                     * We only crash a thread when a bound fiber throws an exception.
                     */
                    print_thrown_exception();
                    kill_task(thread->task); // stays attached, sched_next_task() below frees it

                    enable_context_switch(thread, true);
                    enable_exception_flag(thread, false);
                } else {
                    goto end;
                }
            }

            if(all_tasks_finished(thread)) {
                enable_context_switch(thread, false);
                goto end;
            } else if(thread->task && thread->task->finished) {
                enable_context_switch(thread, true);
            }

            if(hasSignal(thread->signal, tsig_context_switch)
                && !hasSignal(thread->signal, tsig_except)) {
                // the context switch: file the current fiber away, take the next ready one
                sched_next_task(thread);

                if(thread->state == THREAD_KILLED
                    || hasSignal(thread->signal, tsig_kill)) {
                    thread->state = THREAD_KILLED;
                    if(thread->queue != nullptr) {
                        invalidate_queue(thread);
                    }

                    goto end;
                }
            }

        } while (thread->state == THREAD_RUNNING);
    } catch(vm_exception &err) {
        if(thread->state == THREAD_STARTED && thread->currentThread.o != nullptr) {
            assign_numeric_field(resolve_field("exited", thread->currentThread.o)->o, 0, 0);
            assign_numeric_field(resolve_field("exited", thread->currentThread.o)->o, 0, -1);
        }

        if(err.getErr().handlingClass == vm.out_of_memory_except && thread->state == THREAD_CREATED) {
            guard_mutex(thread->mut)
            sendSignal(thread->signal, tsig_kill, 1);
        }

        enable_exception_flag(thread, true);
        if(thread->task && thread->task->boundThread != thread) {
            unboundException = true;
            goto start;
        }
    }

    end:
    TELEMETRY(telemetry_thread_end();)
#ifdef SHARP_PROF_
    if(vm.state != VM_TERMINATED)
        thread_self->tprof->dump();
#endif

//    if(irCount != 0)
//        cout << "instructions executed " << irCount << " overflowed " << overflow << endl;

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
            shutdown();
        } else {
            shutdown_thread(thread);
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

sharp_thread* create_gc_thread() {
    sharp_thread* gc_thread = (sharp_thread*)malloc(
            sizeof(sharp_thread));

    string threadName = "Gc";
    uInt threadId = new_thread_id();
    if(threadId == ILL_THREAD_ID)
        return NULL; // unlikely

    init_struct(gc_thread);
    setup_thread(gc_thread, threadName, threadId, true, nullptr, false);
    gc_thread->priority = THREAD_PRIORITY_LOW;

    post(gc_thread);
    return gc_thread;
}

sharp_thread* create_idle_handler_thread() {
    sharp_thread* idle_thread = (sharp_thread*)malloc(
            sizeof(sharp_thread));

    string threadName = "Idle Handler";
    uInt threadId = new_thread_id();
    if(threadId == ILL_THREAD_ID)
        return NULL; // unlikely

    init_struct(idle_thread);
    setup_thread(idle_thread, threadName, threadId, true, nullptr, false);
    idle_thread->priority = THREAD_PRIORITY_LOW;

    post(idle_thread);
    return idle_thread;
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

    // A thread that has already finished needs no waiting: joining it is a
    // success. (This used to fall through to "join failed", so join() on a
    // short thread reported an error although the work was done.)
    if (thread->state == THREAD_KILLED || thread->exited)
        return RESULT_OK;

    // Running, suspended, or put to sleep by the scheduler (THREAD_SCHED - a
    // thread waiting in delay() or on a lock is as joinable as a running one,
    // but was refused before).
    if (thread->state == THREAD_RUNNING
        || thread->state == THREAD_SUSPENDED
        || thread->state == THREAD_SCHED)
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

        #ifdef __APPLE__
                // macOS version
                struct sched_param param;
                param.sched_priority = pthread_prio;
                pthread_setschedparam(thread->thread, policy, &param);
        #else
                // Linux version
                pthread_setschedprio(thread->thread, pthread_prio);
        #endif

        pthread_attr_destroy(&thAttr);
#endif
        thread->priority = priority;
        return RESULT_OK;
    }

    return RESULT_ILL_PRIORITY_SET;
}

/**
 * Start running the fiber that was put in thread->queue (a new thread's first fiber).
 */
void pop_queue(sharp_thread *thread) {
    guard_mutex(task_mutex)
    fiber *fib = thread->queue;
    thread->queue = nullptr;

    fib->attachedThread = thread;
    thread->task = fib;
    thread->state = THREAD_RUNNING;
    enable_context_switch(thread, false);
    set_task_state(thread, fib, FIB_RUNNING, NO_DELAY);
}

/**
 * Give back a fiber that was handed to this thread but will not be run by it.
 */
void invalidate_queue(sharp_thread *thread) {
    guard_mutex(task_mutex)
    fiber *fib = thread->queue;
    if(fib == nullptr) return;

    thread->queue = nullptr;
    enable_context_switch(thread, false);
    fib->attachedThread = nullptr;
    sched_wake(fib);
}

void enable_context_switch(sharp_thread* thread, bool enable) {
    guard_mutex(thread->mut);
    sendSignal(thread->signal, tsig_context_switch, (enable ? 1 : 0));
}

void enable_exception_flag(sharp_thread* thread, bool enable) {
    guard_mutex(thread->mut);
    sendSignal(thread->signal, tsig_except, (enable ? 1 : 0));
}

void observe_queue(sharp_thread *thread) {
    sched_next_task(thread);
}

/**
 * What a thread sleeping in sched_next_task() waits for: a fiber handed to it
 * (thread->queue), or being told to die or suspend.
 */
bool queue_filled() {
    auto thread = thread_self;
    return thread->queue != nullptr || thread->state == THREAD_KILLED
        || hasSignal(thread->signal, tsig_kill) || hasSignal(thread->signal, tsig_suspend);
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
            NULL,            // default security attributes
            (thread->stackSize), // use default stack size
            vm_thread_entry,   // thread function caller
            thread,             // thread self when thread is created
            0,               // use default creation flags
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
        if(thread->currentThread.o != nullptr
           && IS_CLASS(thread->currentThread.o)) {
            auto threadName = create_object(thread->name.size(), type_int8);
            copy_object(
                    resolve_field(
                        "data",
                        resolve_field("name", thread->currentThread.o)->o
                    ),
                    threadName
            );

            for(Int i = 0; i < thread->name.size(); i++) {
                threadName->HEAD[i] = thread->name[i];
            }
        }
    } else {
        vm.state = VM_RUNNING;
        initialize_main_thread_stack(exeArgs);
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

    if(thread->currentThread.o != nullptr
       && CLASS(thread->currentThread.o->info) == vm.thread_class->address) {
        auto fib = resolve_field("fib", thread->currentThread.o);
        auto main = resolve_field("main", thread->currentThread.o);
        auto name = resolve_field("name", thread->currentThread.o);


        if(fib && main && name) {
            copy_object(
                    fib,
                    create_object(vm.fiber_class)
            );

            copy_object(&thread->task->fiberObject, fib);
            auto fiberName = resolve_field("name", fib->o);
            copy_object(fiberName, name);
            assign_numeric_field(resolve_field("main", fib->o)->o, 0, main->o->HEAD[0]);
            // fiber.id is a native number field (`id : var`), written like `main`
            // above. assign_numeric_class_field() is for number OBJECTS (int,
            // long, ...): it stored the id somewhere else and the first fiber
            // of every new thread kept reporting id 0 - the main fiber's id.
            assign_numeric_field(resolve_field("id", fib->o)->o, 0, thread->task->id);
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

void shutdown_thread(sharp_thread* thread) {
    if (thread->id == main_threadid && thread->task && thread->task->stack != NULL
        && thread->task->stack[vm.mf.threadLocals].obj.o != NULL
        && thread->task->stack[vm.mf.threadLocals].obj.o->type == type_class
        && CLASS(thread->task->stack[vm.mf.threadLocals].obj.o->info) == vm.int_class->address) {
        auto valueField = resolve_field("value", thread->task->stack[vm.mf.threadLocals].obj.o);
        thread->task->exitVal = read_numeric_value(valueField->o, 0);
    }

    print_thrown_exception();

    // give back anything handed to us, kill and free every fiber bound to us
    sched_thread_exit(thread);

    {
        guard_mutex(thread->mut);
        thread->signal = tsig_empty;
        thread->exited = true;
    }

    // Publishing THREAD_KILLED must be the very last thing this function does,
    // with no lock of the thread held: the scheduler monitor frees the thread
    // struct as soon as it sees this state.
    thread->state = THREAD_KILLED;
}

void kill_all_threads() {
    guard_mutex(thread_mutex);
    suspend_all_threads(false);
    _sched_thread *scht = get_sched_threads();
    sharp_thread *thread;

    while(scht != NULL)
    {
        thread = scht->thread;

#ifdef COROUTINE_DEBUGGING
        cout << "Thread " << thread->name << " slept: " << thread->timeSleeping << " switched: " << thread->switched
             << " bound: " << thread->boundFibers << " skipped " << thread->skipped << " actual sleep time: " << (thread->actualSleepTime / 1000)
             << " context switch time (ms) " << NANO_TOMILL(thread->contextSwitchTime) << endl << " time spent locking: " << thread->timeLocking << endl;
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
            else if(thread->state == THREAD_SCHED) {
                send_notification(&thread->queueNotification);
            } else {
                send_interrupt_signal(thread);
            }
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

    // Wait until the new thread has LEFT the "started" state - not until it is
    // seen in the "running" state. A thread does not stay "running": it is put
    // to sleep by the scheduler (THREAD_SCHED), suspends, or simply finishes
    // (THREAD_KILLED). When that happened before this loop looked, the loop
    // never saw THREAD_RUNNING, spun until it gave up and start() returned
    // result_max_spin (255) for a thread that had started fine - which was
    // the usual outcome for a short thread.
    while (thread->state == THREAD_STARTED)
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
    int spins = 0;

    if(sendSignal) send_suspend_signal(thread);
    while (thread->state == THREAD_RUNNING && !thread->suspended)
    {
        if (retryCount++ == sMaxRetries)
        {
            send_suspend_signal(thread);
            retryCount = 0;
            if(thread->state >= THREAD_SUSPENDED)
                return;
            else if(thread->state == THREAD_SCHED) {
                send_notification(&thread->queueNotification);
            } else if(++spins > sMaxSpinCount) 
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
    sharp_thread *current = thread_self;

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
    uint64_t past= Clock::realTimeInNSecs(),now;

    long spinCount = 0;
    long retryCount = 0;

    thread->state = THREAD_SUSPENDED;

    while (thread->suspended)
    {
        if (retryCount++ == sMaxRetries)
        {
            spinCount++;
            retryCount = 0;

            __usleep(1000);
        } else if(thread->state == THREAD_KILLED) {
            thread->suspended = false;
#ifdef COROUTINE_DEBUGGING
        now= Clock::realTimeInNSecs();
        thread->timeSleeping += NANO_TOMICRO(now-past);
#endif
            return;
        } else if(vm.state == VM_SHUTTING_DOWN) {
            thread->suspended = false;
            guard_mutex(thread->mut)
            sendSignal(thread->signal, tsig_kill, 1);
#ifdef COROUTINE_DEBUGGING
        now= Clock::realTimeInNSecs();
        thread->timeSleeping += NANO_TOMICRO(now-past);
#endif
            return;
        }
    }

    {
        guard_mutex(thread->mut);
#ifdef COROUTINE_DEBUGGING
        now= Clock::realTimeInNSecs();
        thread->timeSleeping += NANO_TOMICRO(now-past);
#endif
        thread->state = THREAD_RUNNING;
        sendSignal(thread->signal, tsig_suspend, 0);
    }
}

void resume_all_threads(bool withMarking) {
    guard_mutex(thread_mutex)
    _sched_thread *scht = get_sched_threads();
    sharp_thread *thread;
    sharp_thread *current = thread_self;

    while(scht != NULL)
    {
        thread = scht->thread;
        if(current && thread->id == current->id) {
            scht = scht->next;
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
    guard_mutex(thread_mutex);
    _sched_thread *scht = get_sched_threads();
    sharp_thread *thread;

    while(scht != NULL)
    {
        thread = scht->thread;
        if(thread_self && thread->id == thread_self->id){
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
    sharp_thread *thread = thread_self;

    if(thread)
    {
        TELEMETRY_SCOPE(tm_mode_suspended)
        TELEMETRY_COUNT(tm_suspends)
        thread->suspended = true;
        {
            guard_mutex(thread->mut);
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

    guard_mutex(thread_mutex)
    _sched_thread *thread = get_sched_threads();

    while(thread != NULL)
    {
        if(thread->thread->id == id) return thread->thread;
        thread = thread->next;
    }

    std::lock_guard<recursive_mutex> unschedGuard(unsched_mutex); // threads that were created but not listed yet
    _unsched_thread *unsched_t = unsched_threads;

    while(unsched_t != NULL)
    {
        if(unsched_t->thread->id == id) return unsched_t->thread;
        unsched_t = unsched_t->next;
    }

    return NULL;
}

uInt create_thread(uInt methodAddr, bool daemon) {
    TELEMETRY_COUNT(tm_threads_created)
    int32_t threadId = new_thread_id();
    registers[CMT] = false;
    if(methodAddr < 0 || methodAddr >= vm.mf.methods)
        return RESULT_THREAD_CREATE_FAILED;

    if(threadId == ILL_THREAD_ID)
        return RESULT_NO_THREAD_ID;

    sharp_function* method = &vm.methods[methodAddr];
    if(method->paramSize!=0 || method->fnType != normal_function)
        return RESULT_THREAD_CREATE_FAILED;

    sharp_thread* thread = malloc_struct<sharp_thread>(sizeof(sharp_thread), 1);

    stringstream ss;
    ss << "Thread@" << threadId;
    string name = ss.str();
#ifdef COROUTINE_DEBUGGING
    cout << "new thread " << name << endl;
#endif
    setup_thread(thread, name, threadId, daemon, method, true);

    post(thread);
    registers[CMT] = true;
    return threadId;
}

void setup_thread(sharp_thread *thread, string &name, uInt id, bool daemon, sharp_function* main, bool initializeStack) {
    thread->name = name;
    thread->id = id;
    thread->daemon=daemon;
    if(initializeStack) {
        // not queued: this thread runs it directly as its first fiber (pop_queue)
        auto task = create_task(thread->name, main, nullptr, nullptr, thread, false);
        set_attached_thread(task, thread);
        thread->queue = task;
    }
}

uInt new_thread_id() {
    return ++lastThreadId;
}

bool can_dispose(_sched_thread *scht) {
    return vm.state != VM_SHUTTING_DOWN && scht->thread->state == THREAD_KILLED;
}
