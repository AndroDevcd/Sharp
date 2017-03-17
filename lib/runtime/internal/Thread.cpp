//
// Created by BraxtonN on 2/24/2017.
//

#include "Thread.h"
#include "Exe.h"
#include "Environment.h"
#include "../startup.h"
#include "../interp/vm.h"
#include "../oo/Exception.h"

int32_t Thread::tid = 0;
thread_local Thread* Thread::self = NULL;
list<Thread*>* Thread::threads = NULL;

void Thread::Startup() {
    updateStackFile("starting up thread manager");
    Thread::threads = new list<Thread*>();
    Thread::threads->push_back(new Thread());

    Thread* main = element_at(*Thread::threads, 0);
    main->stack.init();
    main->main = manifest.main;
    main->Create("Main");
}

void Thread::Create(string name, ClassObject* klass, int64_t method) {
    this->monitor = new Monitor();
    this->main = env->getMethodFromClass(klass, method);
    this->name = name;
    this->id = Thread::tid++;
    this->stack.init();
}

void Thread::Create(string name) {
    this->monitor = new Monitor();
    this->name = name;
    this->id = Thread::tid++;
    this->stack.init();
    this->cstack.init();
    this->cstack.thread_stack = &this->stack;
}

Thread *Thread::getThread(int32_t id) {
    for(int32_t i = 0 ; i < threads->size(); i++) {
        Thread* thread = element_at(*threads, i);

        if(thread->id == id)
            return thread;
    }

    return NULL;
}

int Thread::unsuspendThread(int32_t id) {
    Thread* thread = getThread(id);
    if(thread == NULL)
        return 1;

    unsuspendThread(thread);
    return 0;
}

void Thread::suspendSelf() {
    self->suspended = true;
    self->suspendPending = false;

    /*
	 * We call wait upon suspend. This function will
	 * sleep the thread most of the time. notify() or
	 * resumeAllThreads() should be called to revive thread.
	 */
    self->wait();
}

void Thread::wait() {
    const long sMaxRetries = 10000 * 1000;

    long spinCount = 0;
    long retryCount = 0;

    this->state = thread_suspend;

    while (this->suspended)
    {
        if (retryCount++ == sMaxRetries)
        {
            spinCount++;
            retryCount = 0;
#ifdef WIN32_
            Sleep(2);
#endif
#ifdef POSIX_
            usleep(2*POSIX_USEC_INTERVAL);
#endif
        }
    }

    this->state = thread_running;
}

int Thread::start(int32_t id) {
    Thread *thread = getThread(id);

    if (thread == NULL)
        return 1;

    if(thread->state == thread_running)
        return 2;

#ifdef WIN32_
    thread->thread = CreateThread(
            NULL,                   // default security attributes
            0,                      // use default stack size
            &SharpVM::InterpreterThreadStart,       // thread function caller
            thread,                 // thread self when thread is created
            0,                      // use default creation flags
            NULL);
    if(thread->thread != NULL) thread->state = thread_init;
    else return 3; // thread was not started

    return waitForThread(thread);
#endif
#ifdef POSIX_
    if(pthread_create( &thread->thread, NULL, vm->InterpreterThreadStart, (void*) thread))
        return 3; // thread was not started
    else {
        thread->state = thread_init;
        return waitForThread(thread);
    }
#endif

}

int Thread::waitForThread(Thread *thread) {
    while (thread->state == thread_init) { }
    return 0;
}

int Thread::interrupt(int32_t id) {
    if(id == self->id)
        return 1; // cannot interrupt self

    Thread* thread = getThread(id);
    if(thread == NULL)
        return 1;

    return interrupt(thread);
}

void Thread::waitForThreadSuspend(Thread *thread) {
    const int sMaxRetries = 10000000;
    const int sMaxSpinCount = 25;

    int spinCount = 0;
    int retryCount = 0;

    suspendThread(thread);
    while (thread->state == thread_running && !thread->suspended)
    {
        if (retryCount++ == sMaxRetries)
        {
            if(++spinCount >= sMaxSpinCount)
            {
                return; // give up
            } else if(thread->state >= thread_suspend)
                return;
        }
    }
}

void Thread::suspendThread(int32_t id) {
    Thread* thread = getThread(id);
    if(thread == NULL)
        return;

    suspendThread(thread);
}

int Thread::unsuspendThread(Thread *thread) {
    thread->suspended = false;
    return 0;
}

void Thread::suspendThread(Thread *thread) {
    if(thread->id == self->id)
        suspendSelf();
    else {
        thread->event++;
        thread->suspendPending = true;
    }
}

void Thread::term() {
    this->monitor->unlock();
    std::free (this->monitor);
    this->stack.free();
    this->cstack.free();
}

int Thread::join(int32_t id) {
    if (id == self->id)
        return 1;

    Thread* thread = getThread(id);
    if (thread == NULL)
        return 1;

    return threadjoin(thread);
}

int Thread::threadjoin(Thread *thread) {
    if (thread->state == thread_running)
    {
#ifdef WIN32_
        WaitForSingleObject(thread->thread, INFINITE);
        return 0;
#endif
#ifdef POSIX_
        if(pthread_join(thread->thread, NULL))
            return 3;
#endif
    }

    return 2;
}

void Thread::killAll() {
    for(Thread* thread : *threads) {
        if(thread->id != self->id) {
            if(thread->state == thread_running) {
                suspendThread(thread);
                waitForThreadSuspend(thread);

                interrupt(thread);
            } else {
                thread->term();
            }

            std::free (thread); thread = NULL;
        }
    }
}

int Thread::interrupt(Thread *thread) {
    if (thread->state == thread_running)
    {
        waitForThreadSuspend(thread);

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
            vm->Shutdown();
        }
        else
        {
            thread->event++;
            thread->state = thread_killed; // terminate thread
            unsuspendThread(thread);
            return 0;
        }
    }

    return 2;
}

void Thread::shutdown() {
    if(threads != NULL) {
        Thread::killAll();
        Thread::self->term();
        std::free (Thread::self);

        Thread::threads->clear();
        std::free (Thread::threads);
    }
}

void Thread::exit() {
    try {
        this->exitVal = 0;//(int)this->stack.pop()->obj->prim;
    } catch (Exception) {
        this->exitVal = 203;
    }

    this->state = thread_killed;
    if(this->exceptionThrown) {
        // TODO: handle exception
    }
}
