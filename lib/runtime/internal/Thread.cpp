//
// Created by BraxtonN on 2/24/2017.
//

#include "Thread.h"
#include "Exe.h"
#include "../interp/vm.h"

int32_t Thread::tid = 0;
thread_local Thread* thread_self = NULL;
Thread** Thread::threads = NULL;
unsigned int Thread::tp = 0;

void Thread::Startup() {
    Thread::threads = (Thread**)malloc(sizeof(Thread**)*MAX_THREADS);
    for(unsigned int i = 0; i < MAX_THREADS; i++) {
        threads[i] = NULL;
    }

    Thread* main = (Thread*)malloc(
            sizeof(Thread)*1);
    main->main = manifest.main;
    main->Create("Main");
}

void Thread::Create(string name, ClassObject* klass, int64_t method) {
    this->monitor = Monitor();
    this->name.init();

    this->main = env->getMethodFromClass(klass, method);
    this->name = name;
    this->id = Thread::tid++;
    this->stack.init();
    this->cstack.init();
    this->suspendPending = false;
    this->exceptionThrown = false;
    this->suspended = false;
    this->dameon = false;
    this->state = thread_init;
    this->exitVal = 0;

    push_thread(this);
}

void Thread::Create(string name) {
    this->monitor = Monitor();
    this->name.init();

    this->name = name;
    this->id = Thread::tid++;
    this->stack.init();
    this->cstack.init();
    this->cstack.thread_stack = &this->stack;
    this->suspendPending = false;
    this->exceptionThrown = false;
    this->suspended = false;
    this->dameon = false;
    this->state = thread_init;
    this->exitVal = 0;

    push_thread(this);
}

void Thread::CreateDaemon(string) {
    this->monitor = Monitor();
    this->name.init();

    this->name = name;
    this->id = Thread::tid++;
    this->stack=FastStack();
    this->cstack=CallStack();
    this->cstack.thread_stack = &this->stack;
    this->suspendPending = false;
    this->exceptionThrown = false;
    this->suspended = false;
    this->dameon = true;
    this->state = thread_init;
    this->exitVal = 0;

    push_thread(this);
}

void Thread::push_thread(Thread *thread) const {
    bool threadSet = false;
    for(unsigned int i = 0; i < tp; i++) {
        if(threads[i] == NULL) {
            threads[i] = thread;
            threadSet = true;
        }
    }

    if(!threadSet)
        threads[tp++]=thread;
}

Thread *Thread::getThread(int32_t id) {
    for(int32_t i = 0 ; i < tp; i++) {
        Thread* thread = threads[i];

        if(thread != NULL && thread->id == id)
            return thread;
    }

    return NULL;
}

void Thread::suspendSelf() {
    thread_self->suspended = true;
    thread_self->suspendPending = false;

    /*
	 * We call wait upon suspend. This function will
	 * sleep the thread most of the time. notify() or
	 * resumeAllThreads() should be called to revive thread.
	 */
    thread_self->wait();
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
    if(thread->thread == NULL) return 3; // thread was not started
    else
        return 0;
#endif
#ifdef POSIX_
    if(pthread_create( &thread->thread, NULL, vm->InterpreterThreadStart, (void*) thread))
        return 3; // thread was not started
    else {
        return 0;
    }
#endif

}

int Thread::interrupt(int32_t id) {
    if(id == thread_self->id)
        return 1; // cannot interrupt thread_self

    Thread* thread = getThread(id);
    if(thread == NULL || thread->dameon)
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

void Thread::suspendAllThreads() {
    Thread* thread;
    for(unsigned int i= 0; i < tp; i++) {
        thread=threads[i];

        if(thread!=NULL &&
                (thread->id != thread_self->id)){
            suspendThread(thread);
            waitForThreadSuspend(thread);
        }
    }
}

void Thread::resumeAllThreads() {
    Thread* thread;
    for(unsigned int i= 0; i < tp; i++) {
        thread=threads[i];

        if(thread!=NULL &&
           (thread->id != thread_self->id)){
            unsuspendThread(thread);
        }
    }
}

int Thread::unsuspendThread(Thread *thread) {
    thread->suspended = false;
    return 0;
}

void Thread::suspendThread(Thread *thread) {
    if(thread->id == thread_self->id)
        suspendSelf();
    else {
        thread->suspendPending = true;
    }
}

void Thread::term() {
    this->monitor.unlock();
    this->stack.free();
    this->cstack.free();
    this->name.free();
}

int Thread::join(int32_t id) {
    if (id == thread_self->id)
        return 1;

    Thread* thread = getThread(id);
    if (thread == NULL || thread->dameon)
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
    Thread* thread;
    for(unsigned int i = 0; i < tp; i++) {
        thread = threads[i];

        if(thread != NULL && thread->id != thread_self->id) {
            if(thread->state == thread_running) {
                interrupt(thread);
            } else {
                thread->term();
            }
        } else if(thread != NULL){
            thread->term();
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
            thread->state = thread_killed; // terminate thread
            return 0;
        }
    }

    return 2;
}

void Thread::shutdown() {
    if(threads != NULL) {
        Thread::killAll();

        for(unsigned int i = 0; i < tp; i++) {
            if(threads[i] != NULL) {
                std::free(threads[i]); threads[i] = NULL;
            }
        }

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

int Thread::startDaemon(
#ifdef WIN32_
        DWORD WINAPI
#endif
#ifdef POSIX_
void*
#endif
(*threadFunc)(void *), Thread *thread) {
    if (thread == NULL || !thread->dameon)
        return 1;

    if(thread->state == thread_running)
        return 2;

#ifdef WIN32_
    thread->thread = CreateThread(
            NULL,                   // default security attributes
            0,                      // use default stack size
            (LPTHREAD_START_ROUTINE)threadFunc,       // thread function caller
            thread,                 // thread self when thread is created
            0,                      // use default creation flags
            NULL);
    if(thread->thread == NULL) return 3; // thread was not started
    else
        return 0;
#endif
#ifdef POSIX_
    if(pthread_create( &thread->thread, NULL, threadFunc, (void*) thread)!=0)
        return 3; // thread was not started
    else
        return 0;
#endif
}
